#ifndef IV_AERO_CODE_H_
#define IV_AERO_CODE_H_
namespace iv {
namespace aero {

static const int kUndefined = -1;

class Code {
 public:
  Code(const std::vector<uint8_t>& vec,
       int captures, int counters, uint16_t filter)
    : bytes_(vec),
      captures_(captures),
      counters_(counters),
      filter_(filter) { }

  const std::vector<uint8_t>& bytes() const { return bytes_; }

  const uint8_t* data() const { return bytes_.data(); }

  int counters() const { return counters_; }

  int captures() const { return captures_; }

  uint16_t filter() const { return filter_; }

 private:
  std::vector<uint8_t> bytes_;
  int captures_;
  int counters_;
  uint16_t filter_;
};

} }  // namespace iv::aero
#endif  // IV_AERO_CODE_H_