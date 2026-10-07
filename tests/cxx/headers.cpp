// Most of the standard library headers, and a little of each
#include <algorithm>
#include <any>
#include <array>
#include <atomic>
#include <bitset>
#include <cassert>
#include <chrono>
#include <cmath>
#include <complex>
#include <condition_variable>
#include <deque>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <queue>
#include <random>
#include <regex>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <valarray>
#include <variant>
#include <vector>

int main() {
  std::regex re("a+b");
  bool m = std::regex_match("aaab", re);
  std::variant<int, std::string> v = std::string("x");
  std::set<int> s{3, 1, 2};
  std::list<int> l(s.begin(), s.end());
  std::deque<int> d(l.rbegin(), l.rend());
  std::priority_queue<int> pq;
  for (int x : d) pq.push(x);
  std::bitset<8> bits(0xA5);
  std::complex<double> c(1, 2);
  std::mt19937 gen(42);
  std::uniform_int_distribution<int> dist(1, 6);
  std::atomic<int> counter{0};
  std::mutex mtx;
  std::thread t([&] { std::lock_guard<std::mutex> g(mtx); counter += 5; });
  t.join();
  auto fut = std::async(std::launch::deferred, [] { return 7; });
  std::ostringstream os;
  os << std::setw(4) << std::setfill('0') << 42 << " " << std::fixed << std::setprecision(2) << std::abs(c);
  std::string_view sv = "hello world";
  std::any a = 3.5;
  int total = std::accumulate(d.begin(), d.end(), 0);
  std::cout << m << " " << std::get<std::string>(v) << " " << pq.top() << " " << bits.count() << " "
            << os.str() << " " << counter.load() << " " << fut.get() << " " << sv.substr(0, 5) << " "
            << std::any_cast<double>(a) << " " << total << " " << (dist(gen) >= 1) << std::endl;
  return 0;
}
