/*

  Test PBF Output

  The code in this example file is released into the Public Domain.

*/

#include <iomanip>
#include <iostream>
#include <utility>

#include <osmium/io/any_input.hpp>

#include <osmium/io/any_output.hpp>

#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/io/pbf_output.hpp>
#include <osmium/io/error.hpp>
#include <osmium/io/any_compression.hpp>
#include <osmium/io/header.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/osm/box.hpp>
#include <osmium/osm/item_type.hpp>
#include <osmium/osm/location.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/relation.hpp>
#include <osmium/osm/timestamp.hpp>
#include <osmium/osm/types.hpp>
#include <osmium/osm/way.hpp>

inline void maybe_flush(osmium::memory::Buffer& buffer, osmium::io::Writer& writer)
{
  if (buffer.committed() > 800*1024) {
     osmium::memory::Buffer _buffer{1024*1024};
     using std::swap;
     swap(buffer, _buffer);
     writer(std::move(_buffer));
  }

}

inline void test_node(osmium::memory::Buffer& buffer, osmium::io::Writer& writer)
{

   for (int i = 1; i < 1000000; i++) {

      osmium::builder::NodeBuilder builder{buffer};
      osmium::Node& node = builder.object();

      node.set_id(i);
      node.set_version(1);
      node.set_changeset(436543634);
      node.set_deleted(false);
      node.set_uid(4433);
      node.set_timestamp("2016-01-01T00:00:00Z");

      builder.add_user("demo");

      osmium::Location location;
      location.set_lat(50.435254f + i / 100000.0f);
      location.set_lon(-10.1541543f + i / 100000.0f);
      node.set_location(location);

      osmium::builder::TagListBuilder builder_tag{buffer, &builder};
      builder_tag.add_tag("key", "value");

      buffer.commit();

      maybe_flush(buffer, writer);
  }

}



int main(int argc, char* argv[]) {

    int exit_code = 0;

    osmium::io::File outfile("", "pbf");

    try {
        osmium::io::Header header;
        header.set("generator", "osmium_pbfgen");
        header.set("osmosis_replication_timestamp", "2016-05-01T00:00:00Z");

        osmium::io::Writer writer(outfile, header, osmium::io::overwrite::allow);

        osmium::memory::Buffer buffer{1024*1024};

        test_node(buffer, writer);

        // final write
        writer(std::move(buffer));

        writer.close();

    } catch (std::exception& e) {
        std::cerr << e.what() << "\n";
        exit_code = 1;
    }

    return exit_code;
}

