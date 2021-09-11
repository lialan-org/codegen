#include "codegen/variable.hpp"

#include "codegen/arithmetic_ops.hpp"
#include "codegen/compiler.hpp"
#include "codegen/literals.hpp"
#include "codegen/module.hpp"
#include "codegen/module_builder.hpp"
#include "codegen/relational_ops.hpp"
#include "codegen/statements.hpp"
#include "codegen/builtin.hpp"


int32_t table1[][2] = {{0, 1}, {2, 3}, {4, 5}};
int32_t table2[][2] = {{2, 9}, {8, 7}, {6, 5}};

using namespace codegen::literals;

std::unordered_multimap<int, int> hashmap;

void put_val(int32_t i, int32_t j) {
  printf("Putting pair <i, j> = %d, %d\n", i, j);
  hashmap.insert(std::make_pair(i, j));
}

std::vector<int32_t> probe_results;

int64_t probe_hashmap(int32_t i) {
  probe_results.clear();
  printf("Probing index: %d. ", i);
  auto results = hashmap.equal_range(i);
  int64_t counter = 0;
  for (auto it = results.first; it != results.second; ++it) {
    auto table1_index = it->second;
    printf(" Found: %d, ", table1_index);
    probe_results.push_back(table1_index);
    ++counter;
  }
  printf("\n   Total found: %lld\n", counter);
  return counter;
}

int32_t get_probe_result(int64_t index) {
  return probe_results[index];
}

void cleanup_probe() {
  probe_results.clear();
}

void print_probe(int x, int y) {
  printf("    Found join: <%d, %d>\n", x, y);
}

int main() {
  /*
  auto comp = codegen::compiler{};
  auto builder = codegen::module_builder(comp, "hash_join");

  auto put_decl = builder.declare_external_function("put_val", put_val);
  auto probe_decl = builder.declare_external_function("probe_hashmap", probe_hashmap);
  auto cleanup_probe_decl = builder.declare_external_function("cleanup_probe", cleanup_probe);
  auto get_probe_result_decl = builder.declare_external_function("get_probe_result", get_probe_result);
  auto print_probe_decl = builder.declare_external_function("print_probe", print_probe);

  auto build_phase = builder.create_function<void(int32_t*, int32_t)>(
      "build_", [&](codegen::value<int32_t*> table, codegen::value<int32_t> x) {
        auto offset = codegen::variable<uint64_t>("offset", 0_u64);
        auto idx = codegen::variable<int32_t>("idx", 0_i32);

        codegen::while_([&] { return idx.get() < x; },
                        [&] {
                          // extract 2nd column
                          auto val = codegen::load(table + offset.get() + 1_u64);

                          codegen::call(put_decl, idx.get(), val);

                          offset.set(offset.get() + 2_u64);
                          idx.set(idx.get() + 1_i32);
                        });
        codegen::return_();
      });

  auto probe_phase = builder.create_function<void(int32_t*, int32_t)>(
      "probe_", [&](codegen::value<int32_t*> table, codegen::value<int32_t> s) {
        auto offset = codegen::variable<uint64_t>("offset", 0_u64);
        auto idx = codegen::variable<int32_t>("idx", 0_i32);

        codegen::while_([&] { return idx.get() < s; },
                        [&] {
                          // probe extracts 1st column
                          auto val = codegen::load(table + offset.get());

                          auto count = codegen::call(probe_decl, val);

                          auto probe_idx = codegen::variable<int64_t>("probe_idx", 0_i64);

                          codegen::while_([&] { return probe_idx.get() < count; },
                                          [&] {
                                            auto pr = codegen::call(get_probe_result_decl, probe_idx.get());
                                            codegen::call(print_probe_decl, pr, val);

                                            probe_idx.set(probe_idx.get() + 1_i64);
                                          });

                          offset.set(offset.get() + 2_u64);
                          idx.set(idx.get() + 1_i32);
                          codegen::call(cleanup_probe_decl);
                        });
        codegen::return_();
      });

  printf("================= The followings are created LLVM IR: ===============================\n");
  auto module = std::move(builder).build();

  printf("================= The followings are JITed output: ===============================\n");
  auto set_get_ptr = module.get_address(build_phase);
  set_get_ptr((int32_t*)table1, 3);
  auto probe_ptr = module.get_address(probe_phase);
  probe_ptr((int32_t*)table2, 3);
  */
}
