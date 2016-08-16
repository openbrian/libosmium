
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <locale.h>
#include "utf8.h"

#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>

struct BugfinderHandler: public osmium::handler::Handler {

  utf8::iterator< std::string::const_iterator > utf8_begin( const std::string& str )
  { return utf8::iterator<std::string::const_iterator>( str.begin(), str.begin(), str.end() ) ; }

  utf8::iterator< std::string::const_iterator > utf8_end( const std::string& str )
  { return utf8::iterator<std::string::const_iterator>( str.end(), str.begin(), str.end() ) ; }

  inline bool is_invalid_string(const char* s) {

    std::string str(s);

    for( auto it = utf8_begin(str) ; it != utf8_end(str) ; ++it )
      if (*it < 0x20 && *it != 0x9 && *it != 0x0a && *it != 0x0d)
        return true;

    return false;

  }

  inline void osm_object(const osmium::OSMObject& obj) {

    for (const auto& tag : obj.tags())
    {
      if (is_invalid_string(tag.key()) ||
          is_invalid_string(tag.value()))
      std::cout << obj.type() << obj.id() << " v" << obj.version() << " - " << tag.key() << " = " << tag.value() << std::endl;
    }
  }
};


int main(int argc, char* argv[]) {

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " OSMFILE\n";
        exit(1);
    }

    osmium::io::File infile(argv[1]);
    osmium::io::Reader reader(infile);

    BugfinderHandler handler;

    while (osmium::memory::Buffer buffer = reader.read()) {
        osmium::apply(buffer, handler);
    }

    reader.close();
};

