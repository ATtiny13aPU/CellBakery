export module World;
import std;
import osl;
using namespace osl::types;

export template <std::size_t key_max_size = 32, std::size_t value_max_size = 256>
class map_command_t {
private:
	std::array<char, key_max_size> key_array{};
	size_t key_size = 0;

	void set_key(std::string_view sv) {
		if (sv.size() > key_max_size) {
			throw std::invalid_argument("Key too long");
		}
		std::copy(sv.begin(), sv.end(), key_array.begin());
		key_size = sv.size();
	}

	// Прокси-класс для поддержки синтаксиса command["key"] = value
	class Proxy {
	private:
		map_command_t& cmd;
	public:
		Proxy(map_command_t& c) : cmd(c) {}
		template <typename T>
		void operator=(T&& v) {
			cmd.value.store(std::forward<T>(v));
		}
	};

public:
	// value можно сделать теперь публичным
	trivial<value_max_size> value{};

	map_command_t() = default;

	template <typename T>
	map_command_t(std::string_view key, T&& v) {
		set_key(key);
		value.store(std::forward<T>(v));
	}

	template <typename T>
	void set(std::string_view key, T&& v) {
		set_key(key);
		value.store(std::forward<T>(v));
	}

	[[nodiscard]] std::string_view get_key() const {
		return { key_array.data(), key_size };
	}

	map_command_t(const map_command_t& other) = default;
	map_command_t& operator=(const map_command_t& other) = default;

	Proxy operator[](std::string_view key) {
		set_key(key);
		return Proxy(*this);
	}
};

export using map_command_queue_t = osl::lf_mpsc_ring<map_command_t<>, 8192>;
export using map_commands_t = std::vector<map_command_t<>>;

export class WorldAdapter {
public:
	WorldAdapter() : 
		wkv_request_queue_ptr(std::make_unique<map_command_queue_t>()),
		wkv_response_queue_ptr(std::make_unique<map_command_queue_t>()) {}

	struct world_settings_t;
	struct world_render_data_t;
	struct cell_render_data_t;

	void run(const world_settings_t& ws);

	void stop() {
		// Ожидание старта, если ещё не запущен
		while (!is_running.load())
			std::this_thread::yield();
		is_running.store(false);
		wait_to_close();
	}

	// Была запрошена критическая секция
	bool was_request_critical() {
		return is_critical.load();
	}

	// Разблокировка критической секции
	void unblock_critical() {
		is_critical.store(false);
	}

	// Возвращает количество успешно помещённых команд
	size_t push_wkv_commands(map_commands_t& commands) {
		return wkv_request_queue_ptr->push(commands);
	}

	// Возвращает true, если была извлечена хотя бы одна команда
	bool pull_wkv_commands(map_commands_t& commands) {
		return wkv_response_queue_ptr.get()->pop(commands);
	}

	// Захватить новый кадр из тройной буферизации
	world_render_data_t* capture() {
		return world_snapshots.capture();
	}

	// Получить последний доступный кадр из тройной буферизации
	world_render_data_t* last_capture() {
		return world_snapshots.get_current_read();
	}

	// Получить доступ ко всем трём буферам (только для критической секции)
	auto* critical_capture() {
		return world_snapshots.get_storage();
	}


private:
	friend class World;
	void wait_to_close() const {
		while (!is_safe_to_close.load())
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	// Функция самоблокировки мира в критической секции
	void critical_wait() {
		is_critical.store(true);
		// Ждём, пока хост не захочет завершения работы, или критическая задача не завершится
		while (is_running.load() && is_critical.load())
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	std::atomic_bool is_critical;

	std::atomic_bool is_running;
	std::atomic_bool is_safe_to_close;
	/*
		Существуют 3 механизма синхронизации, тройная буферизация и две очереди команд на вход и выход
		request очередь запросов может менять состояние мира, но есть важный нюанс:
		response очередь ответов должна быть спроектирована так,
		чтобы не зависеть от состояния выполения request очереди.
		Это значит, что строить такие команды, асинхронный порядок выполнения которых
		влияет на конечное состояние мира, потому что это нарушает правило детерминированности,
		что в свою очередь не позволит сделать мультиплеер через синхронизацию очереди запросов.
		Точно так же запрещается использовать world_snapshots как источник поведения очереди команд.
	*/
	osl::LF_SPSC_TripleBuffer<world_render_data_t> world_snapshots;
	std::unique_ptr<map_command_queue_t> wkv_request_queue_ptr; // для запросов от хоста к миру
	std::unique_ptr<map_command_queue_t> wkv_response_queue_ptr; // для запросов мира к хосту
};


export struct WorldAdapter::world_settings_t {
	// максимальное число клеток, предполагается динамическое управление памятью
	uint32_t cells_limit;
	// условый размер мира, убивает клетки за пределом
	vec2 world_size;
	// число субшагов на шаг
	uint32_t sub_steps;
};

export struct WorldAdapter::world_render_data_t {
	std::span<cell_render_data_t> cells_vram_storge; // отражение доступной памяти
	std::span<cell_render_data_t> cells; // отражение использованной памяти
	std::unordered_map<std::string, double> bench;

	// время, затраченное на генерацию этого шага мира
	double swap_delta_ms = 0.;

	// ожидаемый индекс кадра при котором завершится работа с vbo на стороне GPU
	uint32_t frame_index = 0;

	// только для чтения
	uint32_t vbo_index = 0; // index в хранилище std::array<shad::vbo, 3>
	uint32_t* frame_index_ptr = nullptr; // указатель на актуальный глобальный счётчик кадров
};

export struct WorldAdapter::cell_render_data_t {
	fvec4 position;	// позиция + скорость в мировых координатах
	fvec4 color;	// RGBA цвет
	fvec4 meta;		// служебные данные: сила.x, сила.y, анимация, радиус
};


using id_t = uint32_t;

// Константы для обозначения состояния клеток
inline constexpr id_t null_id_v = static_cast<id_t>(-1);  // Нет следующей клетки
inline constexpr id_t dead_id_v = static_cast<id_t>(-2);  // Клетка "мёртвая"

export const char* cell_type_names[] = {
	u8"Фагоцит"_cpp17,		// Фагоцит 0
	u8"Жгутоцит"_cpp17,		// Жгутоцит 1
	u8"Фотоцит"_cpp17,		// Фотоцит 2
	u8"Девороцит"_cpp17,	// Девороцит 3
	u8"Липоцит"_cpp17,		// Липоцит 4
	u8"Кератиноцит"_cpp17,	// Кератиноцит 5
	u8"Буецит"_cpp17,		// Буецит 6
	u8"Клейкоцит"_cpp17,	// Клейкоцит 7
	u8"Вироцит"_cpp17,		// Вироцит 8
	u8"Нитроцит"_cpp17,		// Нитроцит 9
	u8"Стереоцит"_cpp17,	// Стереоцит 10
	u8"Сенсеоцит"_cpp17,	// Сенсеоцит 11
	u8"Миоцит"_cpp17,		// Миоцит 12
	u8"Нейроцит"_cpp17,		// Нейроцит 13
	u8"Секроцит"_cpp17,		// Секроцит 14
	u8"Стволоцит"_cpp17,	// Стволоцит 15
	u8"Гамета"_cpp17,		// Гамета 16
	u8"Цилиоцит"_cpp17		// Цилиоцит 17
};

export struct alignas(16) cell_t {
public:
	alignas(16) vec2 force_predict;		// предварительное значение силы (ньютоны)
	alignas(16) vec2 velocity_predict;	// предварительное значение скорости (м/с)
	alignas(16) vec2 pos;				// позиция (метры)
	alignas(16) vec2 force;				// сила (ньютоны)
	alignas(16) vec2 impulse;			// импульс (кг·м/с)
	alignas(16) vec2 velocity;			// скорость (м/с)
	fvec4 color; // RGBA цвет
	frac radius;						// радиус (0.5 м по умолчанию)
	frac weight;						// масса (кг)
	frac angle;
	frac rotate_vel;


	frac forve_abs; // сумма сил взаимодействия (для отладки)

	enum type_t : id_t {
		none = null_id_v,
		phago = 0,	// Фагоцит 0
		flagello,	// Жгутоцит 1
		photo,		// Фотоцит 2
		devoro,		// Девороцит 3
		lipo,		// Липоцит 4
		keratino,	// Кератиноцит 5
		buoyo,		// Буецит 6
		glueo,		// Клейкоцит 7
		viro,		// Вироцит 8
		nitro,		// Нитроцит 9
		stereo,		// Стереоцит 10
		senseo,		// Сенсеоцит 11
		myo,		// Миоцит 12
		neuro,		// Нейроцит 13
		secro,		// Секроцит 14
		stemo,		// Стволоцит 15
		gamete,		// Гамета 16
		cilio		// Цилиоцит 17
	} type;
};

struct line_struct_t {
	line_struct_t() = default;
	frac x = std::numeric_limits<double>::infinity();
	frac y = std::numeric_limits<double>::infinity();
	alignas(8) id_t index = null_id_v;
	frac y_floor = std::numeric_limits<double>::infinity();
	auto operator<=>(const line_struct_t&) const = default;
};

// Класс имплементации мира
class World {
public:
	explicit World(WorldAdapter& wa) : wa(wa) {
		rand.init("3523dgfsdg", 256u);
	}

	// Основной цикл симуляции
	void run(const WorldAdapter::world_settings_t&);

	std::map<std::string, osl::fastMovingAverageW<20>, std::less<>> bench;
private:
	WorldAdapter& wa;

	// Генератор случайных чисел общего назначения
	osl::random rand;

	void update_cells();
	void physics_1(); // Подготовка
	void physics_2(); // Поиск коллизий
	void physics_3(); // Обработка коллизий
	void sync(bool is_main_sync);

	id_t enable_new_cell(); // возвращает id живой неинициализированной клетки
	void disable_cell(const id_t cid); // добавляет id мёрткой клетки в список на удаление

	// Контейнер хранения данных состояния агентов
	osl::pool_container_t<cell_t> cells_pc;
	// Оптимизированный список позиций агентов для поиска коллизий
	std::vector<line_struct_t> lines;
	// Список найденных пар коллизий
	std::vector<std::pair<id_t, id_t>> detected_pair_collision_vec;

	// Очереди команд ключ-значение
	map_commands_t wkv_push_commands; // команды от хоста к миру
	map_commands_t wkv_pull_commands; // команды от мира к хосту

	// Счётчик шагов мира
	uint32_t world_step_counter = (-1);

	// Таймеры
	osl::delta_time_mark dtm;
	osl::delta_time_mark ups;
	osl::delta_time_mark swap_delta;
	osl::update_rate_limiter ups_limiter;
	osl::window::sliding_counter ups_counter{ 100000 };
	osl::window::sliding_counter sync_counter{ 1000 };
	int32_t skip_accum = 0;

	// Счётчик производительности
	uint32_t check_counter = 0u; // считаем общее число проверок потенциальных пар
	uint32_t collision_counter = 0u; // считаем число действительных коолизий

	// Состояние мира
	bool is_paused = true;
	uint32_t cells_limit = 0;
};

// Враппер функция для запуска мира
void WorldAdapter::run(const world_settings_t& ws) {
	World world(*this);
	is_running = true;
	is_safe_to_close = false;
	world.run(ws);
	is_safe_to_close = true;
}