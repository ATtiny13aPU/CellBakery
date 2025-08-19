#pragma once

#include <OSL/include.h>
#include <variant>

using namespace osl;

template <size_t N = 32>
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

typedef osl::LF_MPSC_RingBufferQueue<WorldKeyValueCommand<32>, 8192> WorldKeyValueCommandQueue; // чутьб не умэр пока писал... Xp
typedef std::vector<WorldKeyValueCommand<32>> WorldKeyValueCommands;

class WorldAdapter {
public:

	WorldAdapter() : wkv_queue_ptr(std::make_unique<WorldKeyValueCommandQueue>()) {}
	struct WorldSettings;
	void run(const WorldSettings &ws);

	void stop() {
		isRunning = false;
		wait_to_close();
	}

	size_t pushWKVCommands(WorldKeyValueCommands &commands) {
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


struct WorldAdapter::WorldSettings {
	// максимальное число клеток, предполагается динамическое управление памятью
	size_t cells_limit;
	// условый размер мира, убивает клетки за пределом
	vec2 world_size;
	// число субшагов на шаг
	size_t sub_steps;
};

struct WorldAdapter::RenderData {
	std::vector<RenderCellData> cells;
	std::map<std::string, double> bench;
};

struct WorldAdapter::RenderCellData {
	osl::fvec4 position;	// позиция + скорость в мировых координатах
	osl::fvec4 color;		// RGB + effect
	osl::fvec4 debug;		// Зарезервировано
};
