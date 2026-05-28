/* task_pool_stub.cpp — drop-in replacement for preflate/support/task_pool.cpp.
 *
 * Forces globalTaskPool.extraThreadCount() to return 0, so preflate_decode
 * and preflate_reencode always take their inline path (PreflateDecoderTask
 * / PreflateReencoderTask analyze+encode run on the calling thread, no
 * std::thread is ever constructed, addTask is never called at runtime).
 *
 * Use this in list.txt instead of preflate/support/task_pool.cpp to drop
 * the pthread dependency.
 */

#include "preflate/support/task_pool.h"

TaskPool globalTaskPool;

TaskPool::TaskPool() : _state(INIT), _threadLimit(0) {}
void TaskPool::_init() { _state = RUN; }
TaskPool::~TaskPool() {}
