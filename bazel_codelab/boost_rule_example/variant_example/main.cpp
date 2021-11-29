 #include <boost/variant.hpp>                      //Header only
 #include <string>
 #include <iostream>

using namespace std;

union MyUnion
{
    int i;
    double e;
    string s1;   
};
 

int main(int argc, char* argv[]) {
  boost::variant<double,char, string> v; 
  v = 3.14;
  v = 'A';
  v = "Boost::variant";

  assert(2==v.which());
  return 0;
}
