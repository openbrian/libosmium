#ifndef OSMIUM_IO_FILE_HPP
#define OSMIUM_IO_FILE_HPP

/*

This file is part of Osmium (http://osmcode.org/libosmium).

Copyright 2013-2018 Jochen Topf <jochen@topf.org> and others (see README).

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

*/

#include <osmium/io/error.hpp>
#include <osmium/io/file_compression.hpp>
#include <osmium/io/file_format.hpp>
#include <osmium/util/options.hpp>

#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

namespace osmium {

    /**
     * @brief Everything related to input and output of OSM data.
     */
    namespace io {

        namespace detail {

            inline std::vector<std::string> split(const std::string& in, const char delim) {
                std::vector<std::string> result;
                std::stringstream ss(in);
                std::string item;
                while (std::getline(ss, item, delim)) {
                    result.push_back(item);
                }
                return result;
            }

        } // namespace detail

        /**
         * This class describes an OSM file in one of several different formats.
         *
         * If the filename is empty or "-", this means stdin or stdout is used.
         */
        class File : public osmium::util::Options {

        private:

            std::string m_filename{};

            const char* m_buffer = nullptr;
            size_t m_buffer_size = 0;

            std::string m_format_string;

            file_format m_file_format {file_format::unknown};

            file_compression m_file_compression {file_compression::none};

            bool m_has_multiple_object_versions {false};

        public:

            /**
             * Create File using type and encoding from filename or given
             * format specification.
             *
             * @param filename Filename including suffix. The type and encoding
             *                 of the file will be taken from the suffix.
             *                 An empty filename or "-" means stdin or stdout.
             * @param format File format as string. See the description of the
             *               parse_format() function for details. If this is
             *               empty the format will be deduced from the suffix
             *               of the filename.
             */
            explicit File(std::string filename = "", std::string format = "") :
                m_filename(std::move(filename)),
                m_format_string(std::move(format)) {

                // stdin/stdout
                if (m_filename == "-") {
                    m_filename = "";
                }

                // if filename is a URL, default to XML format
                const std::string protocol{m_filename.substr(0, m_filename.find_first_of(':'))};
                if (protocol == "http" || protocol == "https") {
                    m_file_format = file_format::xml;
                }

                if (m_format_string.empty()) {
                    detect_format_from_suffix(m_filename);
                } else {
                    parse_format(m_format_string);
                }
            }

            /**
             * Create File using buffer pointer and size and type and encoding
             * from given format specification.
             *
             * @param buffer Pointer to buffer with data.
             * @param size   Size of buffer.
             * @param format File format as string. See the description of the
             *               parse_format() function for details.
             */
            explicit File(const char* buffer, size_t size, const std::string& format = "") :
                m_buffer(buffer),
                m_buffer_size(size),
                m_format_string(format) {
                if (!format.empty()) {
                    parse_format(format);
                }
            }

            const char* buffer() const noexcept {
                return m_buffer;
            }

            size_t buffer_size() const noexcept {
                return m_buffer_size;
            }

            void parse_format(const std::string& format) {
                std::vector<std::string> options = detail::split(format, ',');

                // if the first item in the format list doesn't contain
                // an equals sign, it is a format
                if (!options.empty() && options[0].find_first_of('=') == std::string::npos) {
                    detect_format_from_suffix(options[0]);
                    options.erase(options.begin());
                }

                for (auto& option : options) {
                    const size_t pos = option.find_first_of('=');
                    if (pos == std::string::npos) {
                        set(option, true);
                    } else {
                        std::string value{option.substr(pos+1)};
                        option.erase(pos);
                        set(option, value);
                    }
                }

                if (get("history") == "true") {
                    m_has_multiple_object_versions = true;
                } else if (get("history") == "false") {
                    m_has_multiple_object_versions = false;
                }
            }

            void detect_format_from_suffix(const std::string& name) {
                std::vector<std::string> suffixes = detail::split(name, '.');

                if (suffixes.empty()) {
                    return;
                }

                // if the last suffix is one of a known set of compressions,
                // set that compression
                if (suffixes.back() == "gz") {
                    m_file_compression = file_compression::gzip;
                    suffixes.pop_back();
                } else if (suffixes.back() == "bz2") {
                    m_file_compression = file_compression::bzip2;
                    suffixes.pop_back();
                }

                if (suffixes.empty()) {
                    return;
                }

                // if the last suffix is one of a known set of formats,
                // set that format
                if (suffixes.back() == "pbf") {
                    m_file_format = file_format::pbf;
                    suffixes.pop_back();
                } else if (suffixes.back() == "xml") {
                    m_file_format = file_format::xml;
                    suffixes.pop_back();
                } else if (suffixes.back() == "opl") {
                    m_file_format = file_format::opl;
                    suffixes.pop_back();
                } else if (suffixes.back() == "json") {
                    m_file_format = file_format::json;
                    suffixes.pop_back();
                } else if (suffixes.back() == "o5m") {
                    m_file_format = file_format::o5m;
                    suffixes.pop_back();
                } else if (suffixes.back() == "o5c") {
                    m_file_format = file_format::o5m;
                    m_has_multiple_object_versions = true;
                    set("o5c_change_format", true);
                    suffixes.pop_back();
                } else if (suffixes.back() == "debug") {
                    m_file_format = file_format::debug;
                    suffixes.pop_back();
                } else if (suffixes.back() == "blackhole") {
                    m_file_format = file_format::blackhole;
                    suffixes.pop_back();
                }

                if (suffixes.empty()) {
                    return;
                }

                if (suffixes.back() == "osm") {
                    if (m_file_format == file_format::unknown) {
                        m_file_format = file_format::xml;
                    }
                    suffixes.pop_back();
                } else if (suffixes.back() == "osh") {
                    if (m_file_format == file_format::unknown) {
                        m_file_format = file_format::xml;
                    }
                    m_has_multiple_object_versions = true;
                    suffixes.pop_back();
                } else if (suffixes.back() == "osc") {
                    if (m_file_format == file_format::unknown) {
                        m_file_format = file_format::xml;
                    }
                    m_has_multiple_object_versions = true;
                    set("xml_change_format", true);
                    suffixes.pop_back();
                }
            }

            /**
             * Check file format etc. for consistency and throw exception if
             * there is a problem.
             *
             * @throws osmium::io_error
             */
            const File& check() const {
                if (m_file_format == file_format::unknown) {
                    std::string msg{"Could not detect file format"};
                    if (!m_format_string.empty())  {
                        msg += " from format string '";
                        msg += m_format_string;
                        msg += "'";
                    }
                    if (m_filename.empty()) {
                        msg += " for stdin/stdout";
                    } else {
                        msg += " for filename '";
                        msg += m_filename;
                        msg += "'";
                    }
                    msg += ".";
                    throw io_error{msg};
                }
                return *this;
            }

            file_format format() const noexcept {
                return m_file_format;
            }

            File& set_format(file_format format) noexcept {
                m_file_format = format;
                return *this;
            }

            file_compression compression() const noexcept {
                return m_file_compression;
            }

            File& set_compression(file_compression compression) noexcept {
                m_file_compression = compression;
                return *this;
            }

            bool has_multiple_object_versions() const noexcept {
                return m_has_multiple_object_versions;
            }

            File& set_has_multiple_object_versions(bool value) noexcept {
                m_has_multiple_object_versions = value;
                return *this;
            }

            File& filename(const std::string& filename) {
                if (filename == "-") {
                    m_filename = "";
                } else {
                    m_filename = filename;
                }
                return *this;
            }

            const std::string& filename() const noexcept {
                return m_filename;
            }

        }; // class File

    } // namespace io

} // namespace osmium

#endif // OSMIUM_IO_FILE_HPP
