#include <array>
#include <cstdint>
#include <iostream>
#include <unordered_map>

/*
 * Repeat string x times
 */

#define REP2(x)   x         x
#define REP4(x)   REP2(x)   REP2(x)
#define REP8(x)   REP4(x)   REP4(x)
#define REP16(x)  REP8(x)   REP8(x)
#define REP32(x)  REP16(x)  REP16(x)
#define REP64(x)  REP32(x)  REP32(x)
#define REP128(x) REP64(x)  REP64(x)
#define REP256(x) REP128(x) REP128(x)
#define REP512(x) REP256(x) REP256(x)

/*
 * Repeat instruction 512 times
 *
 * Instruction chains are either dependent (on same register) or
 * independent (different registers)
 *
 * Some instructions can't actually be independent, like div
 */

#define INSTRUCTION_ADDL(v)  "addl $1, %%"  v "\n"
#define INSTRUCTION_LEAL(v)  "leal 1(%%" v "), %%" v "\n"
#define INSTRUCTION_IMULL(v) "imull $1, %%" v "\n"

#define INDEP_2_CHAIN(instruction) \
    instruction("r8d")             \
    instruction("r9d")

#define INDEP_4_CHAIN(instruction) \
    instruction("r8d")             \
    instruction("r9d")             \
    instruction("r10d")            \
    instruction("r11d")

#define INDEP_8_CHAIN(instruction) \
    instruction("r8d")             \
    instruction("r9d")             \
    instruction("r10d")            \
    instruction("r11d")            \
    instruction("r12d")            \
    instruction("r13d")            \
    instruction("r14d")            \
    instruction("r15d")

#define DEP_CHAIN_512(instruction)     REP512(instruction("r8d"))
#define INDEP_2_CHAIN_512(instruction) REP256(INDEP_2_CHAIN(instruction))
#define INDEP_4_CHAIN_512(instruction) REP128(INDEP_4_CHAIN(instruction))
#define INDEP_8_CHAIN_512(instruction) REP64(INDEP_8_CHAIN(instruction))

/*
 * Measure the TSC time of the instruction chain into delta
 */
#define MEASURE_CHAIN(chain, delta)   \
    asm volatile(                     \
        "lfence\n"                    \
        "rdtsc\n"                     \
        "lfence\n"                    \
        "movl %%eax, %[delta]\n"      \
                                      \
        chain                         \
                                      \
        "lfence\n"                    \
        "rdtsc\n"                     \
        "lfence\n"                    \
        "subl %[delta], %%eax\n"      \
        "movl %%eax, %[delta]"        \
        : [delta] "=r"(delta)         \
        :                             \
        : "rdx", "rax", "cc",         \
          "r8",  "r9",  "r10", "r11", \
          "r12", "r13", "r14", "r15"  \
    );

/*
 * Macro for function definition of TSC chain profiler
 */
#define MEASURE_FUNCTION(fn_name, chain) \
    static inline uint32_t fn_name() {   \
        uint32_t delta;                  \
        MEASURE_CHAIN(chain, delta);     \
        return delta;                    \
    }

constexpr int known_cycles_per_add{ 1 };
constexpr int samples             { 512 };
constexpr int chain_count         { 512 };

/*
 * Measure TSC time of how long the independent and dependent
 * ADDs take
 *
 * On coffeelake we expect a maximum throughput of 4 independent
 * ADDs per cycle
 */

/*
 * Measure overhead of profiler (rtdsc + lfence) without chain first
 */
MEASURE_FUNCTION(measure_overhead_once, "");

MEASURE_FUNCTION(measure_addl_dependent_once,     DEP_CHAIN_512(INSTRUCTION_ADDL));
MEASURE_FUNCTION(measure_addl_independent_2_once, INDEP_2_CHAIN_512(INSTRUCTION_ADDL));
MEASURE_FUNCTION(measure_addl_independent_4_once, INDEP_4_CHAIN_512(INSTRUCTION_ADDL));
MEASURE_FUNCTION(measure_addl_independent_8_once, INDEP_8_CHAIN_512(INSTRUCTION_ADDL));

MEASURE_FUNCTION(measure_imull_dependent_once,     DEP_CHAIN_512(INSTRUCTION_IMULL));
MEASURE_FUNCTION(measure_imull_independent_2_once, INDEP_2_CHAIN_512(INSTRUCTION_IMULL));
MEASURE_FUNCTION(measure_imull_independent_4_once, INDEP_4_CHAIN_512(INSTRUCTION_IMULL));
MEASURE_FUNCTION(measure_imull_independent_8_once, INDEP_8_CHAIN_512(INSTRUCTION_IMULL));

MEASURE_FUNCTION(measure_leal_dependent_once,     DEP_CHAIN_512(INSTRUCTION_LEAL));
MEASURE_FUNCTION(measure_leal_independent_2_once, INDEP_2_CHAIN_512(INSTRUCTION_LEAL));
MEASURE_FUNCTION(measure_leal_independent_4_once, INDEP_4_CHAIN_512(INSTRUCTION_LEAL));
MEASURE_FUNCTION(measure_leal_independent_8_once, INDEP_8_CHAIN_512(INSTRUCTION_LEAL));

static int get_mode(const std::array<uint32_t, samples>& values) {
    std::unordered_map<uint32_t, int> histogram;

    for (uint32_t value : values)
        ++histogram[value];

    uint32_t best_value{};
    int best_count{ -1 };

    for (const auto& [value, count] : histogram) {
        if (count > best_count) {
            best_value = value;
            best_count = count;
        }
    }

    return static_cast<int>(best_value);
}

template<class Fn>
static int get_mode_from_samples(Fn measure_once) {
    std::array<uint32_t, samples> deltas{};

    for (uint32_t& delta : deltas)
        delta = measure_once();

    return get_mode(deltas);
}

static double adjusted_ticks_per_instruction(int raw, int overhead) {
    return static_cast<double>(raw - overhead) / chain_count;
}

int main() {
    int overhead = get_mode_from_samples(measure_overhead_once);

    double addl_dep_ticks     = adjusted_ticks_per_instruction(get_mode_from_samples(measure_addl_dependent_once),     overhead);
    double addl_indep_2_ticks = adjusted_ticks_per_instruction(get_mode_from_samples(measure_addl_independent_2_once), overhead);
    double addl_indep_4_ticks = adjusted_ticks_per_instruction(get_mode_from_samples(measure_addl_independent_4_once), overhead);
    double addl_indep_8_ticks = adjusted_ticks_per_instruction(get_mode_from_samples(measure_addl_independent_8_once), overhead);

    double tick_cycle_ratio = (known_cycles_per_add / addl_dep_ticks);

    double addl_dep_cycles     = known_cycles_per_add;
    double addl_indep_2_cycles = tick_cycle_ratio * addl_indep_2_ticks;
    double addl_indep_4_cycles = tick_cycle_ratio * addl_indep_4_ticks;
    double addl_indep_8_cycles = tick_cycle_ratio * addl_indep_8_ticks;

    double imull_dep_ticks     = adjusted_ticks_per_instruction(get_mode_from_samples(measure_imull_dependent_once),     overhead);
    double imull_indep_2_ticks = adjusted_ticks_per_instruction(get_mode_from_samples(measure_imull_independent_2_once), overhead);
    double imull_indep_4_ticks = adjusted_ticks_per_instruction(get_mode_from_samples(measure_imull_independent_4_once), overhead);
    double imull_indep_8_ticks = adjusted_ticks_per_instruction(get_mode_from_samples(measure_imull_independent_8_once), overhead);

    double imull_dep_cycles     = tick_cycle_ratio * imull_dep_ticks;
    double imull_indep_2_cycles = tick_cycle_ratio * imull_indep_2_ticks;
    double imull_indep_4_cycles = tick_cycle_ratio * imull_indep_4_ticks;
    double imull_indep_8_cycles = tick_cycle_ratio * imull_indep_8_ticks;

    double leal_dep_ticks     = adjusted_ticks_per_instruction(get_mode_from_samples(measure_leal_dependent_once),     overhead);
    double leal_indep_2_ticks = adjusted_ticks_per_instruction(get_mode_from_samples(measure_leal_independent_2_once), overhead);
    double leal_indep_4_ticks = adjusted_ticks_per_instruction(get_mode_from_samples(measure_leal_independent_4_once), overhead);
    double leal_indep_8_ticks = adjusted_ticks_per_instruction(get_mode_from_samples(measure_leal_independent_8_once), overhead);

    double leal_dep_cycles     = tick_cycle_ratio * leal_dep_ticks;
    double leal_indep_2_cycles = tick_cycle_ratio * leal_indep_2_ticks;
    double leal_indep_4_cycles = tick_cycle_ratio * leal_indep_4_ticks;
    double leal_indep_8_cycles = tick_cycle_ratio * leal_indep_8_ticks;

    std::cout << "dependent ticks/add:              " << addl_dep_ticks     << '\n';
    std::cout << "independent ticks/add (2 indep.): " << addl_indep_2_ticks << '\n';
    std::cout << "independent ticks/add (4 indep.): " << addl_indep_4_ticks << '\n';
    std::cout << "independent ticks/add (8 indep.): " << addl_indep_8_ticks << '\n';
    std::cout << '\n';
    std::cout << "dependent cycles/add:                   " << addl_dep_cycles     << '\n';
    std::cout << "est. independent cycles/add (2 indep.): " << addl_indep_2_cycles << '\n';
    std::cout << "est. independent cycles/add (4 indep.): " << addl_indep_4_cycles << '\n';
    std::cout << "est. independent cycles/add (8 indep.): " << addl_indep_8_cycles << '\n';
    std::cout << "\n\n";

    std::cout << "dependent ticks/imul:              " << imull_dep_ticks     << '\n';
    std::cout << "independent ticks/imul (2 indep.): " << imull_indep_2_ticks << '\n';
    std::cout << "independent ticks/imul (4 indep.): " << imull_indep_4_ticks << '\n';
    std::cout << "independent ticks/imul (8 indep.): " << imull_indep_8_ticks << '\n';
    std::cout << '\n';
    std::cout << "dependent cycles/imul:                   " << imull_dep_cycles     << '\n';
    std::cout << "est. independent cycles/imul (2 indep.): " << imull_indep_2_cycles << '\n';
    std::cout << "est. independent cycles/imul (4 indep.): " << imull_indep_4_cycles << '\n';
    std::cout << "est. independent cycles/imul (8 indep.): " << imull_indep_8_cycles << '\n';
    std::cout << "\n\n";

    std::cout << "dependent ticks/lea:              " << leal_dep_ticks     << '\n';
    std::cout << "independent ticks/lea (2 indep.): " << leal_indep_2_ticks << '\n';
    std::cout << "independent ticks/lea (4 indep.): " << leal_indep_4_ticks << '\n';
    std::cout << "independent ticks/lea (8 indep.): " << leal_indep_8_ticks << '\n';
    std::cout << '\n';
    std::cout << "dependent cycles/lea:                   " << leal_dep_cycles     << '\n';
    std::cout << "est. independent cycles/lea (2 indep.): " << leal_indep_2_cycles << '\n';
    std::cout << "est. independent cycles/lea (4 indep.): " << leal_indep_4_cycles << '\n';
    std::cout << "est. independent cycles/lea (8 indep.): " << leal_indep_8_cycles << '\n';
}
