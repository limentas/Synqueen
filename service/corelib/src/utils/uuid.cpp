#include "uuid.hpp"

#include <algorithm>
#include <array>
#include <random>
#include <stduuid/uuid.h>

namespace synqueen {

std::string generateUUID() {
  std::random_device rd;
  auto seed_data = std::array<int, std::mt19937::state_size>{};
  std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
  std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
  std::mt19937 generator(seq);
  uuids::uuid_random_generator gen{generator};
  const auto id = gen();
  return uuids::to_string(id);
}

} // namespace synqueen
