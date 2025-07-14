//
// Created by cory on 1/16/25.
//

#include <thread>
#include <atomic>
#include "network/tcpserver.hpp"
#include "server.hpp"
#include "clamsutil.hpp"

std::atomic<bool> running;
Logger logs;
std::thread chat_runner;

Logger& logger()
{
    return logs;
}

bool is_running()
{
    return running.load();
}

static void tick()
{

}

// tick scheduler pulled from Minestom
constexpr double TICKS_PER_SECOND = 20.0;
constexpr uint64_t TICK_TIME_NANOS = 1000000000L / TICKS_PER_SECOND;
constexpr uint64_t SERVER_MAX_TICK_CATCH_UP = 5;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
constexpr uint64_t SLEEP_THRESHOLD = 17000; // ns
#else
constexpr uint64_t SLEEP_THRESHOLD = 2000; // ns
#endif

using std::chrono::high_resolution_clock;
using std::chrono::milliseconds;
using std::this_thread::sleep_for;

using std::chrono::duration_cast;

static uint64_t nanotime()
{
    return high_resolution_clock::now().time_since_epoch().count();
}

/**
 * Tries to sleep until the given time
 * @param until Nanosecond timescale time stamp about when to wake up
 */
static inline void sleep(uint64_t until)
{
    uint64_t now;

    // While the time we should stop is still in the future
    while ((now = nanotime()) < until)
    {
        // Sleep until we get past the future.

        // Time until we need to stop
        uint64_t remaining_ns = until - now;

        // If it's pretty far, we can sleep off some of that time.
        // If it's pretty close, we keep this alive (o.o)
        if (remaining_ns >= SLEEP_THRESHOLD)
        {
            // Sleep less the closer we are to the next tick
            uint64_t half_remaining_ms = remaining_ns / 2000000L; // convert to ms and half
            sleep_for(milliseconds(half_remaining_ms));
        }
        // I thought that once the remaining_ns gets past the
        // sleep threshold, (think about it like the time right
        // before your train comes, you don't want to fall asleep
        // and miss it), this would run a bunch of times unnecessarily,
        // but after benchmarking this, it turns out that it doesn't.
        // The call to nanotime() must take more time than I thought,
        // because most of the time the loop only ran once. Rarely did
        // I see it run twice. And sometimes it even ran 0 times, it
        // perfectly timed. So this is good to keep this way.
    }
}

static void loop()
{
    uint64_t ticks = 0;
    // You can think of this as the time when ticks started occurring.
    // This is used to predict the time in ns when any tick should start.
    uint64_t base_time = nanotime();
    uint64_t next_start;

    while (running)
    {
        tick();
        ++ticks;

        // The time when the next tick should begin
        next_start = base_time + TICK_TIME_NANOS * ticks;
        sleep(next_start);

        // Check if the server can not keep up with the tick rate
        // if it gets too far behind, reset the ticks & base_time
        // to avoid running too many ticks at once. This assumes
        // that the server is running slow for all ticks, and that
        // the ticks that would've had to have run would be slower
        // than their allotted time (50 ms).
        if (nanotime() > next_start + TICK_TIME_NANOS * SERVER_MAX_TICK_CATCH_UP)
        {
            base_time = nanotime();
            ticks = 0;
        }
    }
}

static bool init(unsigned short port)
{
    if (!tcp_init(port))
    {
        std::cerr << "Unable to post initialize the TCP server" << std::endl;
        return false;
    }

    return true;
}

static void chat_thread()
{
    std::string in;

    while (running)
    {
        std::cout << "> " << std::flush;
        std::getline(std::cin, in);

        if (in == "stop")
        {
            std::cout << "Stopping... " << std::endl;
            server_stop();
        }
    }
}

void server_start(unsigned short port)
{
    auto start = high_resolution_clock::now();
    debug(logs.info("Debug: 1");)

    logs.info("Starting Minecraft server on *:%u", port);

    if (running)
    {
        logs.err("Invocation of server_start(unsigned short) when already running!");
        return;
    }

    running = true;

    if (!init(port))
    {
        logs.err("Failed to initialize!");
        return;
    }

    tcp_start();

    logs.info("Done (%u ms)", duration_cast<milliseconds>(high_resolution_clock::now() - start).count());

    chat_runner = std::thread(chat_thread);
    loop();
    chat_runner.join();
}

void server_stop()
{
    running = false;

    tcp_stop();
}
