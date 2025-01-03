/**
 * @file log.cpp
 * @brief The implementation file for the logger.
 */
#include "logger/log.h"

namespace rk {
namespace log {

/**
 * Checks whether the log queue has messages or the endLoop flag is true. If either are true, it will return true,
 * stop the condition variable from waiting, and resume execution of the log loop.
 */
bool condVarPredicate() {
    std::unique_lock<std::mutex> endLoopLock(endLoopMtx);
    return !logQueue.empty() || endLoop;
}

/**
 * This function will continously check the log queue for new messages and print them via std::cout.
 * If no logs are in the queue, it'll wait until new ones get added.
 * It will end the loop once the endLoop flag is set to true and the queue is empty.
 */
void logQueueLoop() {
    std::string msg;
    while (true) {
        std::unique_lock<std::mutex> logLock(logQueueMutex);
        if (!logQueue.empty()) {
            msg = logQueue.front();
            logQueue.pop();
            if (!msg.empty()){
                std::cout << msg;
                logFile << msg;
                msg.clear();
            }
        }
        else {
            std::unique_lock<std::mutex> endLoopLock(endLoopMtx);
            if (!endLoop) {
                endLoopLock.unlock();
                cv.wait(logLock, rk::log::condVarPredicate);
            }
            else {
                break;
            }
        }
    }
}

/**
 * Creates the thread that will run the log loop function - logQueueLoop.
 */
std::thread startLogThread() {
    std::thread logThread(logQueueLoop);
    return logThread;
}

/**
 * Ends the log thread that was passed in by first setting the endLoop flag to
 * true. This will allow the log loop to exit. Then, it will join the thread.
 */
void endLogThread(std::thread& thread) {
    {
        std::lock_guard<std::mutex> lock(endLoopMtx);
        endLoop = true;
        cv.notify_all();
    }

    if (thread.joinable()) {
        thread.join();
    }
}

/**
 * Closes the log file by using std::ofstream::close().
 */
void closeLogFile() {
    logFile.close();
}

} // namespace log
} // namespace rk
