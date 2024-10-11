#include <iostream>
#include <boost/asio.hpp>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <locale>

using boost::asio::ip::tcp;

// Trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
        [](unsigned char ch) { return !std::isspace(ch); }));
}

// Trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
        [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
}

// Trim from both ends (in place)
static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

std::string read_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void write_file(const std::string& filename, const std::string& content) {
    std::ofstream file(filename, std::ios::app); // Append mode
    if (file) {
        file << content << "\n";
    }
}

std::string url_decode(const std::string& str) {
    std::string result;
    char ch;
    int i, ii;
    for (i=0; i<str.length(); i++) {
        if (int(str[i]) == 37) {
            sscanf(str.substr(i+1,2).c_str(), "%x", &ii);
            ch = static_cast<char>(ii);
            result += ch;
            i = i+2;
        } else {
            result += str[i];
        }
    }
    return result;
}

std::vector<std::string> read_posts(const std::string& filename) {
    std::vector<std::string> posts;
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line)) {
        posts.push_back(line);
    }
    return posts;
}

void handle_request(tcp::socket& socket) {
    try {
        boost::asio::streambuf buffer;
        boost::system::error_code error;
        boost::asio::read_until(socket, buffer, "\r\n\r\n", error);

        if (error && error != boost::asio::error::eof) {
            std::cerr << "Error reading request: " << error.message() << "\n";
            return;
        }

        std::istream request_stream(&buffer);
        std::string request_line;
        std::getline(request_stream, request_line);
        std::string method, path, version;
        std::istringstream request_line_stream(request_line);
        request_line_stream >> method >> path >> version;

        // Read headers (not used in this simple example)
        // std::string header;
        // while (std::getline(request_stream, header) && header != "\r") {
        //     // Parse headers if needed
        // }

        // Sanitize the path
        // Remove query parameters
        size_t query_pos = path.find('?');
        if (query_pos != std::string::npos) {
            path = path.substr(0, query_pos);
        }

        // Trim whitespace and carriage returns
        trim(path);

        // Debugging output
        std::cout << "Method: " << method << ", Path: " << path << ", Version: " << version << "\n";

        if (method == "GET") {
            if (path == "/") {
                // Serve the main page
                std::string content = read_file("static/index.html");
                std::string response = "HTTP/1.1 200 OK\r\nContent-Length: " + 
                    std::to_string(content.length()) + "\r\nContent-Type: text/html\r\n\r\n" + content;
                boost::asio::write(socket, boost::asio::buffer(response));
            } else if (path == "/posts") {
                // Return the list of posts as plain text
                auto posts = read_posts("posts.txt");
                std::string content;
                for (const auto& post : posts) {
                    content += post + "\n";
                }
                std::string response = "HTTP/1.1 200 OK\r\nContent-Length: " + 
                    std::to_string(content.length()) + "\r\nContent-Type: text/plain\r\n\r\n" + content;
                boost::asio::write(socket, boost::asio::buffer(response));
            } else {
                // 404 Not Found
                std::string response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
                boost::asio::write(socket, boost::asio::buffer(response));
            }
        } else if (method == "POST" && path == "/add_post") {
            // Read the rest of the request to get the body
            std::string body;
            if (buffer.size() > 0) {
                std::istream body_stream(&buffer);
                std::getline(body_stream, body, '\0');
            }

            // URL decode the body
            body = url_decode(body);

            // Extract the 'post' parameter
            std::string post_content;
            size_t pos = body.find("post=");
            if (pos != std::string::npos) {
                post_content = body.substr(pos + 5);
            }

            // Save the post to the file
            if (!post_content.empty()) {
                write_file("posts.txt", post_content);
                // Respond with 200 OK
                std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK";
                boost::asio::write(socket, boost::asio::buffer(response));
            } else {
                // Bad Request
                std::string response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
                boost::asio::write(socket, boost::asio::buffer(response));
            }
        } else {
            // Method Not Allowed
            std::string response = "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\n\r\n";
            boost::asio::write(socket, boost::asio::buffer(response));
        }
    } catch (std::exception& e) {
        std::cerr << "Exception in handle_request: " << e.what() << "\n";
    }
}

int main() {
    try {
        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 8080));

        std::cout << "Server is running on port 8080..." << std::endl;

        while (true) {
            tcp::socket socket(io_context);
            acceptor.accept(socket);
            handle_request(socket);
        }
    } catch (std::exception& e) {
        std::cerr << "Exception in main: " << e.what() << "\n";
    }

    return 0;
}

