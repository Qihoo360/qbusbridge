#ifndef LUTIL_BENCHMARKER_H
#define LUTIL_BENCHMARKER_H

#include <string>
#include <iostream>
#include <map>
//---------------------------------------
namespace LUtil {
    class BenchMarker {
        public:
            BenchMarker():
                tag_(""),
       start_time_ms_(0),
       is_show_detail_(true) {
                }

            void start(const char *tag) {
                tag_ = (NULL != tag ? tag : "");
                struct timeval now_time; 
                gettimeofday(&now_time, NULL); 
                start_time_ms_ = ((long)now_time.tv_sec)*1000+(long)now_time.tv_usec/1000;
            }

            void stop() {
                struct timeval now_time; 
                gettimeofday(&now_time, NULL); 
                long stop = ((long)now_time.tv_sec)*1000+(long)now_time.tv_usec/1000;
                total_time_ms_[tag_] = stop - start_time_ms_;
            }

            void release() {
                long total = 0;
                for (std::map<std::string, long>::iterator i = total_time_ms_.begin(), 
                            e = total_time_ms_.end();
                            i != e; ++i) {
                    total += i->second;
                    if (is_show_detail_) {
                        std::cout << "BenchMarker | " << i->first << " | " << i->second << std::endl;
                    }
                }
                total_time_ms_.clear();

                std::cout << "BenchMarker | total: " << total << std::endl;
            }

        private:
            std::string tag_;
            long start_time_ms_;
            bool is_show_detail_;
            std::map<std::string, long> total_time_ms_;
    };
}

#endif
