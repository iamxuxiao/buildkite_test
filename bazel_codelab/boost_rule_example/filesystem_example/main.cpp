
#include <boost/filesystem.hpp>
#include <iostream>


using namespace std;

int main(int argc, char* argv[]) {
  boost::filesystem::path full_path( boost::filesystem::current_path() );
  std::cout << "Current path is : " << full_path << std::endl;

  if (!boost::filesystem::exists("./filesystem_example/myfile.txt")) {
    std::cout << "Can't the file!\n";
  }else{
    std::cout << "File Found" << std::endl;    
  }
  
  return 0;
}
