#pragma once

namespace qbus {
namespace kafka {

class noncopyable {
   protected:
    noncopyable() {}
    ~noncopyable() {}

   private:
    noncopyable(const noncopyable&);
    noncopyable& operator=(const noncopyable&);
};

}  // namespace kafka
}  // namespace qbus
