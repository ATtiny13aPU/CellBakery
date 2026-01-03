export module World;
import std;
import osl;
using namespace osl::types;

export template <size_t N = 32>
class WorldKeyValueCommand {
private:
	std::array<char, N> key_array{};
	size_t key_size = 0;
	std::variant<uint64_t, double> value{};

	void set_key(std::string_view sv) {
		if (sv.size() > N) {
			throw std::invalid_argument("Key too long");
		}
		std::copy(sv.begin(), sv.end(), key_array.begin());
		key_size = sv.size();
	}

	template <typename T>
	void set_value(const T& v) {
		if constexpr (std::is_integral_v<T>) {
			value = static_cast<uint64_t>(v);
		}
		else if constexpr (std::is_floating_point_v<T>) {
			value = static_cast<double>(v);
		}
		else {
			static_assert(std::is_same_v<T, uint64_t> || std::is_same_v<T, double>,
				"Value must be integer or float/double");
			value = v;
		}
	}

	// Прокси-класс для поддержки синтаксиса command["key"] = value
	class Proxy {
	private:
		WorldKeyValueCommand& cmd;
	public:
		Proxy(WorldKeyValueCommand& c) : cmd(c) {}
		template <typename T>
		void operator=(const T& v) {
			cmd.set_value(v);
		}
	};

public:
	WorldKeyValueCommand() = default;

	template <typename T>
	WorldKeyValueCommand(std::string_view key, T v) {
		set_key(key);
		set_value(v);
	}

	template <typename T>
	void set(std::string_view key, T v) {
		set_key(key);
		set_value(v);
	}

	std::string_view get_key() const {
		return { key_array.data(), key_size };
	}

	const std::variant<uint64_t, double>& get_value() const {
		return value;
	}

	WorldKeyValueCommand(const WorldKeyValueCommand& other) {
		*this = other;
	}

	WorldKeyValueCommand& operator=(const WorldKeyValueCommand& other) {
		if (this != &other) {
			set_key(other.get_key());
			value = other.value;
		}
		return *this;
	}

	Proxy operator[](std::string_view key) {
		set_key(key);
		return Proxy(*this);
	}
};

// чутьб не умэр пока писал... Xp
export using WorldKeyValueCommandQueue = osl::LF_MPSC_RingBufferQueue<WorldKeyValueCommand<32>, 8192>;
export using WorldKeyValueCommands = std::vector<WorldKeyValueCommand<32>>;

export class WorldAdapter {
public:

	WorldAdapter() : wkv_queue_ptr(std::make_unique<WorldKeyValueCommandQueue>()) {}
	struct WorldSettings;
	void run(const WorldSettings& ws);

	void stop() {
		while (!isRunning.load())
			std::this_thread::yield();
		isRunning.store(false);
		wait_to_close();
	}

	size_t pushWKVCommands(WorldKeyValueCommands& commands) {
		return wkv_queue_ptr->push(commands);
	}

	struct RenderData;
	struct RenderCellData;
	inline const RenderData* capture() {
		return world_data_snapshots.capture();
	}

private:
	friend class World;
	void wait_to_close() const {
		while (!isSafeToClose)
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	std::atomic_bool isRunning;
	std::atomic_bool isSafeToClose;
	osl::LF_SPSC_TripleBuffer<RenderData> world_data_snapshots;
	std::unique_ptr<WorldKeyValueCommandQueue> wkv_queue_ptr;
};


export struct WorldAdapter::WorldSettings {
	// максимальное число клеток, предполагается динамическое управление памятью
	uint32_t cells_limit;
	// условый размер мира, убивает клетки за пределом
	vec2 world_size;
	// число субшагов на шаг
	uint32_t sub_steps;
};

export struct WorldAdapter::RenderData {
	std::vector<RenderCellData> cells;
	std::map<std::string, double> bench;
};

export struct WorldAdapter::RenderCellData {
	fvec4 position;	// позиция + скорость в мировых координатах
	fvec4 color;		// RGB + effect
	fvec4 meta;		// Зарезервировано
};


using id = uint32_t;

// Константы для обозначения состояния клеток
inline const id nullID = static_cast<id>(-1);  // Нет следующей клетки
inline const id deadID = static_cast<id>(-2);  // Клетка "мёртвая"

class Cell {
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
};

// Класс имплементации мира
class World {
public:
	explicit World(WorldAdapter& wa) : wa(wa) {
		rand.init("3523dgfsdg", 256u);
	}

	// Основной цикл симуляции
	void run(const WorldAdapter::WorldSettings&);

	std::map<std::string, osl::fastMovingAverageW<120>, std::less<>> bench;
private:
	WorldAdapter& wa;

	osl::random rand;
	void update_cells();

	osl::UpdateRateLimiter ups_limiter;
	osl::PoolContainer<Cell> cells_pc;
};

// Враппер функция для запуска мира
void WorldAdapter::run(const WorldSettings& ws) {
	World world(*this);
	isRunning = true;
	isSafeToClose = false;
	world.run(ws);
	isSafeToClose = true;
}