#include "sysc/tracing/sc_coverage_trace.h"
#include "sysc/datatypes/bit/sc_bv_base.h"
#include "sysc/datatypes/bit/sc_logic.h"
#include "sysc/datatypes/bit/sc_lv_base.h"
#include "sysc/datatypes/int/sc_int_base.h"
#include "sysc/datatypes/int/sc_signed.h"
#include "sysc/datatypes/int/sc_uint_base.h"
#include "sysc/datatypes/int/sc_unsigned.h"
#include "sysc/tracing/sc_trace.h"
#include "sysc/tracing/sc_tracing_ids.h"

constexpr std::uint8_t leading1Pos(unsigned int val) { return (8 * sizeof(unsigned int)) - __builtin_clz(val) - 1; }
template <typename T, typename std::enable_if<std::is_integral<T>::value || std::is_enum<T>::value, int>::type = 0>
constexpr std::uint8_t logB2(T val) {
  return static_cast<unsigned>(val) == 0 ? 0
                                         : (leading1Pos(static_cast<unsigned>(val)) +
                                            ((static_cast<unsigned>(val) & (static_cast<unsigned>(val) - 1)) ? 1 : 0));
}

namespace sc_core {
  extern const sc_time& sc_time_stamp();

  template <typename T>
  class cover_info : public cover_info_base {
   private:
    static constexpr bool SpecialZeroBucketOK = !std::is_same<sc_dt::sc_logic, T>::value &&
                                                !std::is_same<bool, T>::value && !std::is_same<sc_dt::sc_bit, T>::value;
    static constexpr std::size_t MaxNumBuckets    = 16;
    static constexpr std::size_t LogMaxNumBuckets = logB2(MaxNumBuckets);
    static constexpr bool        isSigned = std::is_signed<T>::value || std::is_base_of<sc_dt::sc_signed, T>::value ||
                                     std::is_base_of<sc_dt::sc_int_base, T>::value;

   public:
    struct cover_bucket {
      std::size_t      count;
      sc_core::sc_time totalTime;
      sc_core::sc_time minTime;
      sc_core::sc_time maxTime;
      bool             minSet;

      cover_bucket() : count(0), minTime(sc_core::sc_time::from_value(~0ULL)), minSet(false) {}

      void increment(sc_core::sc_time diff) {
        ++count;
        totalTime += diff;
        if(!minSet) {
          minSet  = true;
          minTime = diff;
        } else {
          if(diff < minTime) minTime = diff;
        }
        if(diff > maxTime) maxTime = diff;
      }

      std::string str() {
        std::stringstream ss;
        ss << count << ";";
        ss << (minSet ? minTime.to_default_time_units() : 0) << ";";
        ss << maxTime.to_default_time_units() << ";";
        ss << totalTime.to_default_time_units();
        return ss.str();
      }
    };

   protected:
    bool                      bucketPerVal;
    std::int32_t              count;
    std::vector<cover_bucket> buckets;
    sc_core::sc_time          lastMod;
    std::size_t               curBucket;
    std::size_t               numBits;
    std::size_t               numBuckets;
    std::size_t               logNumBuckets;
    bool                      hasSpecialZeroBucket;

    const T& curVal;
    T        lastVal;

   public:
    template <typename U = T, typename std::enable_if<std::is_integral<U>::value, int>::type = 0>
    cover_info(const std::string& name, const U& object, std::size_t numBits, bool _bucketPerVal = false)
      : cover_info_base(name),
        bucketPerVal(_bucketPerVal && (numBits <= 10)),
        count(0),
        curBucket(0),
        numBits(numBits),
        curVal(object),
        lastVal(object) {}

    template <typename U = T,
              typename std::enable_if<
                (std::is_base_of<sc_dt::sc_bv_base, U>::value || std::is_base_of<sc_dt::sc_lv_base, U>::value ||
                 std::is_base_of<sc_dt::sc_uint_base, U>::value || std::is_base_of<sc_dt::sc_int_base, U>::value ||
                 std::is_base_of<sc_dt::sc_signed, U>::value || std::is_base_of<sc_dt::sc_unsigned, U>::value),
                int>::type = 0>
    cover_info(const std::string& name, const U& object, std::size_t numBits, bool _bucketPerVal = false)
      : cover_info_base(name),
        bucketPerVal(_bucketPerVal && (numBits <= 10)),
        count(0),
        curBucket(0),
        numBits(numBits),
        curVal(object),
        lastVal(object.length()) {}

    template <typename U = T, typename std::enable_if<std::is_same<sc_dt::sc_logic, U>::value, int>::type = 0>
    cover_info(const std::string& name, const U& object, bool _bucketPerVal = false)
      : cover_info_base(name),
        bucketPerVal(false),
        count(0),
        buckets(4),
        curBucket(sc_dt::Log_X),
        numBits(1),
        numBuckets(4),
        logNumBuckets(2),
        curVal(object),
        lastVal(object) {
      static_assert(!SpecialZeroBucketOK, "Shouldn't have zero bucket for sc_logic");
    }

    template <typename U = T, typename std::enable_if<std::is_same<sc_dt::sc_bit, U>::value, int>::type = 0>
    cover_info(const std::string& name, const U& object, bool _bucketPerVal = false)
      : cover_info_base(name),
        bucketPerVal(false),
        count(0),
        buckets(2),
        curBucket(0),
        numBits(1),
        numBuckets(2),
        logNumBuckets(1),
        curVal(object),
        lastVal(object) {
      static_assert(!SpecialZeroBucketOK, "Shouldn't have zero bucket for sc_bit");
    }

    inline std::int32_t getCoverage() const override { return count; }
    inline std::int32_t getNumBuckets() const override { return bucketPerVal ? (1 << numBits) : numBuckets; }
    inline std::int32_t getBucket(std::size_t i) const override {
      return (i >= 0 && i < buckets.size()) ? buckets[i].count : 0;
    }
    inline bool changed() const override { return curVal != lastVal; }

    std::string getCSV() const override {
      std::stringstream ss;
      ss << name << "," << count;
      for(auto b : buckets) {
        ss << "," << b.str();
      }
      return ss.str();
    }

    void incrementCount() override {
      if(changed()) {
        ++count;
        incrementBucket(getBucket());
        lastVal = curVal;
      }
    }

    void initialize() override {
      if(bucketPerVal && numBits > 10) {
        std::cerr << "bucketPerVal not supported for more than 10 bit signal\n";
        std::exit(EXIT_FAILURE);
      }

      if(!std::is_same<sc_dt::sc_logic, T>::value && !std::is_same<sc_dt::sc_bit, T>::value) {
        bucketPerVal |= (numBits <= LogMaxNumBuckets);
        hasSpecialZeroBucket = SpecialZeroBucketOK && !bucketPerVal;

        numBuckets    = bucketPerVal ? (1 << numBits) : (MaxNumBuckets + hasSpecialZeroBucket);
        logNumBuckets = logB2(numBuckets - hasSpecialZeroBucket);

        for(std::size_t i = 0; i < numBuckets; ++i) {
          buckets.emplace_back();
        }
      }
      lastVal = curVal;
    }

   private:
    void incrementBucket(size_t i) {
      sc_core::sc_time diff = sc_core::sc_time_stamp() - lastMod;
      buckets[curBucket].increment(diff);
      curBucket = i;
      lastMod   = sc_core::sc_time_stamp();
    }

    template <typename U = T, typename std::enable_if<std::is_same<sc_dt::sc_logic, U>::value, int>::type = 0>
    std::int32_t getBucket() {
      return static_cast<std::int32_t>(curVal.value());
    }

    template <typename U = T, typename std::enable_if<std::is_same<sc_dt::sc_bit, U>::value, int>::type = 0>
    std::int32_t getBucket() {
      return static_cast<std::int32_t>(curVal.to_bool());
    }

    template <typename U = T, typename std::enable_if<std::is_integral<U>::value, int>::type = 0>
    std::int32_t getBucket() {
      if(hasSpecialZeroBucket && curVal == 0) return isSigned ? (numBuckets / 2) : 0;

      std::int32_t shift = numBits - logNumBuckets;
      return (bucketPerVal ? curVal : (curVal >> shift)) + (isSigned * (numBuckets / 2)) + hasSpecialZeroBucket;
    }

    template <typename U = T,
              typename std::enable_if<
                (std::is_base_of<sc_dt::sc_bv_base, U>::value || std::is_base_of<sc_dt::sc_lv_base, U>::value ||
                 std::is_base_of<sc_dt::sc_uint_base, U>::value || std::is_base_of<sc_dt::sc_int_base, U>::value ||
                 std::is_base_of<sc_dt::sc_signed, U>::value || std::is_base_of<sc_dt::sc_unsigned, U>::value),
                int>::type = 0>
    std::int32_t getBucket() {
      if(hasSpecialZeroBucket && curVal.nor_reduce()) return isSigned ? (numBuckets / 2) : 0;

      std::int32_t bucket = 0;
      if(isSigned) {
        bucket = bucketPerVal ? curVal.to_int() : curVal.range(numBits - 1, numBits - logNumBuckets).to_int();
        bucket += numBuckets / 2;
      } else {
        bucket = bucketPerVal ? curVal.to_uint() : curVal.range(numBits - 1, numBits - logNumBuckets).to_uint();
      }
      bucket += hasSpecialZeroBucket;
      return bucket;
    }
  };

  coverage_trace_file::coverage_trace_file(const char* name) : sc_trace_file_base(name, "csv") {}

  coverage_trace_file::~coverage_trace_file() {
    // Actually write out all the coverage information
    for(auto& covObj : coveredObjs) {
      std::fputs((covObj->getCSV() + "\n").c_str(), this->fp);
      delete covObj;
    }
  }

  void coverage_trace_file::add_object(const std::string& name, cover_info_base* obj) {
    if(objNames.count(name) != 0) {
      std::stringstream msg;
      msg << "object with name " << name << " already added to trace file";
      SC_REPORT_WARNING(SC_ID_TRACING_DUPLICATE_OBJECT_, msg.str().c_str());
    } else {
      coveredObjs.push_back(obj);
    }
  }

#define TRACE_OBJECT(type)                                                       \
  void coverage_trace_file::trace(const type& object, const std::string& name) { \
    add_object(name, new cover_info<type>(name, object));                        \
  }

#define TRACE_OBJECT_UNSUPPORTED(type)                                            \
  void coverage_trace_file::trace(const type& object, const std::string& name) {  \
    std::stringstream msg;                                                        \
    msg << "Currently no support for covering " << #type << " objects: " << name; \
    SC_REPORT_WARNING(sc_core::SC_ID_TRACING_OBJECT_IGNORED_, msg.str().c_str()); \
  }

#define TRACE_OBJECT_NBITS(type, nbits)                                          \
  void coverage_trace_file::trace(const type& object, const std::string& name) { \
    add_object(name, new cover_info<type>(name, object, nbits));                 \
  }

#define TRACE_OBJECT_WIDTH(type)                                                            \
  void coverage_trace_file::trace(const type& object, const std::string& name, int width) { \
    add_object(name, new cover_info<type>(name, object, width));                            \
  }

  TRACE_OBJECT_NBITS(bool, 1);  
  TRACE_OBJECT(sc_dt::sc_bit);
  TRACE_OBJECT(sc_dt::sc_logic);
  TRACE_OBJECT_WIDTH(unsigned char);
  TRACE_OBJECT_WIDTH(unsigned short);
  TRACE_OBJECT_WIDTH(unsigned int);
  TRACE_OBJECT_WIDTH(unsigned long);
  TRACE_OBJECT_WIDTH(char);
  TRACE_OBJECT_WIDTH(short);
  TRACE_OBJECT_WIDTH(int);
  TRACE_OBJECT_WIDTH(long);
  TRACE_OBJECT_WIDTH(sc_dt::int64);
  TRACE_OBJECT_WIDTH(sc_dt::uint64);
  TRACE_OBJECT_UNSUPPORTED(float);
  TRACE_OBJECT_UNSUPPORTED(double);
  TRACE_OBJECT_NBITS(sc_dt::sc_uint_base, object.length());
  TRACE_OBJECT_NBITS(sc_dt::sc_int_base, object.length());
  TRACE_OBJECT_NBITS(sc_dt::sc_unsigned, object.length());
  TRACE_OBJECT_NBITS(sc_dt::sc_signed, object.length());
  TRACE_OBJECT_UNSUPPORTED(sc_dt::sc_fxval);
  TRACE_OBJECT_UNSUPPORTED(sc_dt::sc_fxval_fast);
  TRACE_OBJECT_UNSUPPORTED(sc_dt::sc_fxnum);
  TRACE_OBJECT_UNSUPPORTED(sc_dt::sc_fxnum_fast);
  TRACE_OBJECT_NBITS(sc_dt::sc_bv_base, object.length());
  TRACE_OBJECT_NBITS(sc_dt::sc_lv_base, object.length());

  void coverage_trace_file::trace(const unsigned int& object, const std::string& name, const char** enum_literals) {
    std::stringstream msg;
    msg << "Currently no support for covering enum objects: " << name;
    SC_REPORT_WARNING(sc_core::SC_ID_TRACING_OBJECT_IGNORED_, msg.str().c_str());
  }

  void coverage_trace_file::do_initialize() {
    for(auto& covObj : coveredObjs) {
      covObj->initialize();
    }
  }

  void coverage_trace_file::cycle(bool delta_cycle) {
    // Trace delta cycles only when enabled
    if(!delta_cycles() && delta_cycle) return;

    // Check for initialization
    if(initialize()) {
      return;
    };

    for(auto& covObj : coveredObjs) {
      if(covObj->changed()) {
        covObj->incrementCount();
      }
    }
  }

  sc_trace_file* sc_create_coverage_trace_file(const char* name) {
    coverage_trace_file* tf = new coverage_trace_file(name);
    return tf;
  }

  void sc_close_coverage_trace_file(sc_core::sc_trace_file* tf) {
    coverage_trace_file* cov_tf = static_cast<coverage_trace_file*>(tf);
    delete cov_tf;
  }

}  // namespace sc_core
