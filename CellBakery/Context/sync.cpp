module;

#include "monolith_std_osl_header.h";
#include "monolith_ogl_imgui_header.h";

module Context;

inline void Context::sync() {

	// TODO: рефакторинг
	const auto world_state = world.capture();
	if (world_state) {
		// обновление графики
		const auto& cells = world_state->cells;
		framePerUpdate.push(frame_counter - last_update_frame, 1.);
		last_update_frame = frame_counter;
		cellsMesh.loadFrom(cells.data(), cells.size());
		time_lerp -= 1.;
		delta_time_lerp = (1. - time_lerp) / framePerUpdate.get();
		if (!(delta_time_lerp > 0. && delta_time_lerp < 1.))
			delta_time_lerp = 0., time_lerp = 1.;


		// отображение состояния мира
		std::stringstream ss;

		const auto& b = world_state->bench;
		ss << std::fixed << std::setprecision(2)
			<< b.at("gap1") << " + " << b.at("gap2") << " = " << b.at("gap")
			<< "ms  gcc:" << b.at("gapcc") << " avrc:" << b.at("avr_c") << " ups:" << 1000. / b.at("mspu")
			<< ' ' << time_lerp << ' ' << delta_time_lerp * 100.;

		window.setTitle(ss.str().c_str());
	}
	time_lerp += delta_time_lerp;

}