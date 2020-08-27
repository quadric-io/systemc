#pragma once

#include <set>
#include <sstream>
#include <vector>

#include "sysc/tracing/sc_trace_file_base.h"

namespace sc_core {
  class cover_info_base {
   public:
    cover_info_base(const std::string& name) : name(name) {}
    virtual std::int32_t getCoverage() const            = 0;
    virtual std::int32_t getNumBuckets() const          = 0;
    virtual std::int32_t getBucket(std::size_t i) const = 0;
    virtual std::string  getCSV() const                 = 0;
    virtual void         incrementCount()               = 0;
    virtual bool         changed() const                = 0;
    virtual void         initialize()                   = 0;

    // Necessary for when deleting using pointer to base class
    virtual ~cover_info_base() {}

    const std::string name;    
   protected:
  };

  template <typename T>
  class cover_info;

  class coverage_trace_file : public sc_core::sc_trace_file_base {
   public:
    coverage_trace_file(const char* name);
    ~coverage_trace_file();

   protected:
    // These are all virtual functions in sc_trace_file and
    // they need to be defined here.

    void trace(const sc_event& object, const std::string& name);
    void trace(const sc_time& object, const std::string& name);
    void trace(const bool& object, const std::string& name);
    void trace(const sc_dt::sc_bit& object, const std::string& name);
    void trace(const sc_dt::sc_logic& object, const std::string& name);
    void trace(const unsigned char& object, const std::string& name, int width);
    void trace(const unsigned short& object, const std::string& name, int width);
    void trace(const unsigned int& object, const std::string& name, int width);
    void trace(const unsigned long& object, const std::string& name, int width);
    void trace(const char& object, const std::string& name, int width);
    void trace(const short& object, const std::string& name, int width);
    void trace(const int& object, const std::string& name, int width);
    void trace(const long& object, const std::string& name, int width);
    void trace(const sc_dt::int64& object, const std::string& name, int width);
    void trace(const sc_dt::uint64& object, const std::string& name, int width);
    void trace(const float& object, const std::string& name);
    void trace(const double& object, const std::string& name);
    void trace(const sc_dt::sc_uint_base& object, const std::string& name);
    void trace(const sc_dt::sc_int_base& object, const std::string& name);
    void trace(const sc_dt::sc_unsigned& object, const std::string& name);
    void trace(const sc_dt::sc_signed& object, const std::string& name);
    void trace(const sc_dt::sc_fxval& object, const std::string& name);
    void trace(const sc_dt::sc_fxval_fast& object, const std::string& name);
    void trace(const sc_dt::sc_fxnum& object, const std::string& name);
    void trace(const sc_dt::sc_fxnum_fast& object, const std::string& name);
    void trace(const sc_dt::sc_bv_base& object, const std::string& name);
    void trace(const sc_dt::sc_lv_base& object, const std::string& name);
    void trace(const unsigned int& object, const std::string& name, const char** enum_literals);

    void do_initialize();
    void write_comment(const std::string& comment) {}
    void cycle(bool delta_cycle);

   private:
    std::vector<cover_info_base*> coveredObjs;
    std::set<std::string>         objNames;

    void add_object(const std::string& name, cover_info_base* obj);
  };
}  // namespace sc_core
