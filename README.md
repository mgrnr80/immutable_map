# immutable_map

An immutable ordered map implemented in C++.
Work in progress.

## Usage
```C
// construction
immutable_map<int, double> map0;
// insert and removal
auto map1 = map0.insert(std::make_pair(10, 3.14));
auto map2 = map1.insert(std::make_pair(15, 6.28));
auto map3 = map2.insert(std::make_pair(20, 3.14));
auto map4 = map3.erase(15);
// access
double val = map4.at(10);
// lookup
bool has_val = map4.contains(11);
auto size = map4.size();
// iteration
map4.foreach([](const std::pair<int,double>& kvp) { std::cout << "[" << kvp.first << "]"; });
```
