// qbus_pause_resume_example.cc:
// Each time \p kMessageBatchSize messages of the same topic arrived, pause the
// topic for \p kPauseSeconds seconds, then resume it.
#include "qbus_consumer.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <iostream>
#include <map>
using namespace std;

#define CHECK_RETVAL(expr)                             \
    if (!expr) {                                       \
        cerr << "[ERROR] " #expr << " failed" << endl; \
        exit(1);                                       \
    }

static int kMessageBatchSize = 2;
static int kPauseSeconds = 3;
static string kConfigPath = "./consumer.config";
static string kLogPath = "./qbus_pause_resume_example.log";

static qbus::QbusConsumer kQbusConsumer;

static volatile bool run = true;
static void stop(int) { run = false; }

typedef std::vector<std::string> StringVector;

// Helper functions
static const char* now();
static StringVector split(const char* s, const char* delim = ", \t\n");
static int strToInt(const char* s);

class MyCallback : public qbus::QbusConsumerCallback {
   public:
    virtual void deliveryMsg(const string& topic, const char* msg, const size_t msg_len) const {
        if (msg_id_map.find(topic) == msg_id_map.end()) msg_id_map[topic] = 0;

        size_t& id = msg_id_map[topic];
        cout << topic << "[" << id++ << "] | " << string(msg, msg_len) << endl;

        if (id % kMessageBatchSize == 0) {
            CHECK_RETVAL(kQbusConsumer.pause(StringVector(1, topic)));
            cout << now() << " | Pause consuming " << topic << endl;

            pthread_t tid;
            pthread_create(&tid, NULL, &MyCallback::resumeTopic, new string(topic));
            pthread_detach(tid);
        }
    }

   private:
    // topic name => latest msg id
    mutable std::map<std::string, size_t> msg_id_map;

    static void* resumeTopic(void* arg) {
        string* topic = static_cast<string*>(arg);
        assert(topic);

        sleep(kPauseSeconds);

        cout << now() << " | Resume consuming " << *topic << endl;
        CHECK_RETVAL(kQbusConsumer.resume(StringVector(1, *topic)));

        delete topic;
        return NULL;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        fprintf(stderr,
                "Usage: %s topics [batch-size] [pause-secs] [conf-path] [log-path]\n"
                "  topics     comma-seperated topics\n"
                "  batch-size message count before each pause/resume\n"
                "  pause-secs seconds to sleep between pause and resume\n"
                "  conf-path  config path (default: %s)\n"
                "  log-path   log path (default: %s)\n"
                "Note: bootstrap.servers and group.id must be configured!\n",
                argv[0], kConfigPath.c_str(), kLogPath.c_str());
        exit(1);
    }

    StringVector topics = split(argv[1]);
    cout << "%% topics to consume: ";
    for (size_t i = 0; i < topics.size(); i++) cout << "[" << i << "] " << topics[i] << " ";

    if (argc > 2) kMessageBatchSize = strToInt(argv[2]);
    if (argc > 3) kPauseSeconds = strToInt(argv[3]);
    if (argc > 4) kConfigPath = argv[4];
    if (argc > 5) kLogPath = argv[5];
    cout << "\n%% Arguments:" << endl;
    cout << "batch-size: " << kMessageBatchSize << endl;
    cout << "pause-secs: " << kPauseSeconds << endl;
    cout << "conf-path: " << kConfigPath << endl;
    cout << "log-path: " << kLogPath << endl;
    cout << "----------------------------------------------" << endl;

    MyCallback my_callback;
    CHECK_RETVAL(kQbusConsumer.init("", kLogPath, kConfigPath, my_callback));
    CHECK_RETVAL(kQbusConsumer.subscribe("", topics));
    CHECK_RETVAL(kQbusConsumer.start());
    cout << "%% Consumer started... (Press Ctrl+C to stop)" << endl;

    signal(SIGINT, stop);
    while (run) {
        sleep(1);
    }

    kQbusConsumer.stop();
    cout << "\n%% Consumer stopped." << endl;
    return 0;
}

static const char* now() {
    static char buf[128];
    struct timeval tv;
    gettimeofday(&tv, NULL);

    struct tm* tmp = localtime(&tv.tv_sec);
    assert(tmp);
    assert(strftime(buf, sizeof(buf), "%F %T", tmp));

    size_t len = strlen(buf);
    snprintf(buf + len, sizeof(buf) - len, ".%03d", static_cast<int>(tv.tv_usec / 1000));

    return buf;
}

static StringVector split(const char* s, const char* delim) {
    size_t len = strlen(s);
    char* buf = new char[len + 1];
    strncpy(buf, s, len);
    buf[len] = '\0';

    StringVector result;
    char* token = strtok(buf, delim);
    while (token) {
        result.push_back(token);
        token = strtok(NULL, delim);
    }

    delete[] buf;
    return result;
}

static int strToInt(const char* s) {
    char* endptr;
    errno = 0;
    long result = strtol(s, &endptr, 10);
    if (result <= INT_MIN || result >= INT_MAX) {
        cerr << "strToInt \"" << s << "\": overflows" << endl;
        exit(1);
    }
    if (errno != 0) {
        cerr << "strToInt \"" << s << "\": " << strerror(errno) << endl;
        exit(1);
    }
    return result;
}
