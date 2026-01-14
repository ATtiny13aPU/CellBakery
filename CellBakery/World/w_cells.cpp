module World;
import osl;
using namespace osl::types;

// Обновление состояния агентов
void World::update_cells() {
	id_t eid = 0;
	for (auto& cid : cells_pc.enabled) {
		//if (rand.u8q() > 253)
		//	disable_cell(eid);
		eid++;
	}
	cells_pc.erase();
}

// Создаёт новую живую клетку и возвращает её id
// Так же добавляет её в линию
id_t World::enable_new_cell() {
	// Добавляем элемент в конец линии
	lines.push_back(line_struct_t{});
	const id_t id = cells_pc.get_new();
	lines.back().index = id;
	return id;
}

// Удаляет клетку с заданным id из линии (принимает id из enabled)
// Помечает клетку как неактивную
void World::disable_cell(const id_t eid) {
	if (eid >= cells_pc.enabled.size()) {
		std::println("W: warning, an attempt to deactivate an inactive cell.");
		return;
	}
	id_t cid = cells_pc.enabled[eid];
	// помечаем как неактивную
	// чтобы lines увидел это и удалил на следующем обновлении
	if (cid < cells_pc.storage.size())
		cells_pc.storage[cid].type = cell_t::type_t::none;
	cells_pc.erase(eid);
}