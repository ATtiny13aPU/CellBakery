export module World;
import std;
import osl;
using namespace osl::types;

export template <std::size_t key_max_size = 32, std::size_t value_max_size = 256>
class WorldKeyValueCommand {
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
		WorldKeyValueCommand& cmd;
	public:
		Proxy(WorldKeyValueCommand& c) : cmd(c) {}
		template <typename T>
		void operator=(T&& v) {
			cmd.value.store(std::forward<T>(v));
		}
	};

public:
	// value можно сделать теперь публичным
	trivial<value_max_size> value{};

	WorldKeyValueCommand() = default;

	template <typename T>
	WorldKeyValueCommand(std::string_view key, T&& v) {
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

	WorldKeyValueCommand(const WorldKeyValueCommand& other) = default;
	WorldKeyValueCommand& operator=(const WorldKeyValueCommand& other) = default;

	Proxy operator[](std::string_view key) {
		set_key(key);
		return Proxy(*this);
	}
};

// чутьб не умэр пока писал... Xp
export using WorldKeyValueCommandQueue = osl::LF_MPSC_RingBufferQueue<WorldKeyValueCommand<>, 8192>;
export using WorldKeyValueCommands = std::vector<WorldKeyValueCommand<>>;

export class WorldAdapter {
public:
	WorldAdapter() : 
		wkv_request_queue_ptr(std::make_unique<WorldKeyValueCommandQueue>()),
		wkv_response_queue_ptr(std::make_unique<WorldKeyValueCommandQueue>()) {}

	struct WorldSettings;
	struct world_render_data_t;
	struct cell_render_data_t;

	void run(const WorldSettings& ws);

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
	size_t push_wkv_commands(WorldKeyValueCommands& commands) {
		return wkv_request_queue_ptr->push(commands);
	}

	// Возвращает true, если была извлечена хотя бы одна команда
	bool pull_wkv_commands(WorldKeyValueCommands& commands) {
		return wkv_response_queue_ptr.get()->pop(commands);
	}

	// Захватить новый кадр из тройной буферизации
	const world_render_data_t* capture() {
		return world_snapshots.capture();
	}

	// Получить последний доступный кадр из тройной буферизации
	const world_render_data_t* last_capture() {
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
	std::unique_ptr<WorldKeyValueCommandQueue> wkv_request_queue_ptr; // для запросов от хоста к миру
	std::unique_ptr<WorldKeyValueCommandQueue> wkv_response_queue_ptr; // для запросов мира к хосту
};


export struct WorldAdapter::WorldSettings {
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
};

export struct WorldAdapter::cell_render_data_t {
	fvec4 position;	// позиция + скорость в мировых координатах
	fvec4 color;	// RGBA цвет
	fvec4 meta;		// служебные данные: сила.x, сила.y, анимация, радиус
};


using id = uint32_t;

// Константы для обозначения состояния клеток
inline const id nullID = static_cast<id>(-1);  // Нет следующей клетки
inline const id deadID = static_cast<id>(-2);  // Клетка "мёртвая"

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

export class cell_t {
public:
	fvec4 color;
	vec2 pos;       // позиция (метры)
	vec2 force;     // сила (ньютоны)
	vec2 impulse;   // импульс (кг·м/с)
	vec2 velocity;  // скорость (м/с)
	frac weight;    // масса (кг)
	frac angle;
	frac rotate_vel;
	frac radius;    // радиус (0.5 м по умолчанию)

	enum type_t : id {
		phago,		// Фагоцит 0
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

// Класс имплементации мира
class World {
public:
	explicit World(WorldAdapter& wa) : wa(wa) {
		rand.init("3523dgfsdg", 256u);
	}

	// Основной цикл симуляции
	void run(const WorldAdapter::WorldSettings&);

	std::map<std::string, osl::fastMovingAverageW<20>, std::less<>> bench;
private:
	WorldAdapter& wa;

	// Генератор случайных чисел общего назначения
	osl::random rand;

	void update_cells();
	void physics_1();
	void physics_2();
	void sync();

	// Контейнер хранения данных состояния агентов
	osl::PoolContainer<cell_t> cells_pc;
	// Оптимизированный список позиций агентов для поиска коллизий
	std::vector<std::pair<vec2, id>> lines;
	// Очереди команд ключ-значение
	WorldKeyValueCommands wkv_push_commands; // команды от хоста к миру
	WorldKeyValueCommands wkv_pull_commands; // команды от мира к хосту

	// Счётчик шагов мира
	uint32_t world_step_counter = (-1);

	// Таймеры
	osl::DeltaTimeMark dtm;
	osl::DeltaTimeMark ups;
	osl::UpdateRateLimiter ups_limiter;

	// Состояние мира
	bool is_paused = false;
	uint32_t cells_limit = 0;
};

// Враппер функция для запуска мира
void WorldAdapter::run(const WorldSettings& ws) {
	World world(*this);
	is_running = true;
	is_safe_to_close = false;
	world.run(ws);
	is_safe_to_close = true;
}