#include "RequestHandler.h"
#include <fstream>
#include <filesystem>
#include <sstream>

// URL-decode percent-escapes
static std::string urlDecode(const std::string& in) {
    std::string out; out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        if (in[i] == '%' && i + 2 < in.size()) {
            int val = 0;
            std::istringstream hex{ in.substr(i + 1, 2) };
            if (hex >> std::hex >> val) {
                out.push_back(static_cast<char>(val));
                i += 2;
            }
        } else if (in[i] == '+') {
            out.push_back(' ');
        } else {
            out.push_back(in[i]);
        }
    }
    return out;
}

HttpResponse RequestHandler::handlePut() const {
    // raw filename from URI
    std::string raw = request.uri.substr(request.uri.find_last_of('/') + 1);
    std::string name = urlDecode(raw);

    namespace fs = std::filesystem;
    fs::path route(routePath);
    fs::path dir = route.parent_path();
    fs::path out = dir / name;

    std::ofstream ofs(out, std::ios::binary);
    if (!ofs.is_open()) {
        return HttpResponse(500);  // can’t open file
    }

    ofs.write(request.body.data(), request.body.size());
    if (ofs.fail()) {
        return HttpResponse(500);  // write error
    }

    return HttpResponse(201);  // Created
}