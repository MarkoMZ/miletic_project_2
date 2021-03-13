

/*

This file contains an class which is able to determine which MIME-Type a file extension has. 
 
*/


#ifndef MIME_TYPES_HPP
#define MIME_TYPES_HPP

#include <iostream>
#include <map>
#include <string>

#include "http_client.hpp"

using namespace std;

class MIMEType {
    public:

        // Variable to save MIMETYPE
        string currentMIMEType;

        /* 
           Initialize a list which contains a map 
           The map has the most commonly used file-extensions
           mapped to their equivalent MIME-Type
        */ 
        map<string, string> MIMETYPE {
            {".aac", "audio/aac"},
            {".abw", "application/x-abiword"},
            {".arc", "application/x-freearc"},
            {".avi", "video/x-msvideo"},
            {".bin", "application/octet-stream"}, 
            {".bmp", "image/bmp"}, 
            {".bz", "application/x-bzip"}, 
            {".bz2", "application/x-bzip2"}, 
            {".css", "text/css"}, 
            {".csv", "text/csv"}, 
            {".doc", "application/msword"}, 
            {".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"}, 
            {".gz", "application/gzip"}, 
            {".gif", "image/gif"},
            {".htm", "text/html"}, 
            {".html", "text/html"}, 
            {".jar", "application/java-archive"}, 
            {".jpeg", "image/jpeg"}, 
            {".jpg", "image/jpeg"}, 
            {".js", "text/javascript"}, 
            {".json", "application/json"}, 
            {".jsonld", "application/ld+json"}, 
            {".mjs", "text/javascript"},
            {".mp3", "audio/mpeg"}, 
            {".mpeg", "video/mpeg"}, 
            {".mpkg", "application/vnd.apple.installer+xml"}, 
            {".odt", "application/vnd.oasis.opendocument.text"}, 
            {".oga", "	audio/ogg"}, 
            {".ogv", "video/ogg"},
            {".ogx", "application/ogg"},
            {".opus", "audio/opus"},
            {".otf", "font/otf"},
            {".png", "image/png"},
            {".pdf", "application/pdf"},
            {".php", "application/x-httpd-php"},
            {".ppt", "application/vnd.ms-powerpoint"},
            {".pptx", "	application/vnd.openxmlformats-officedocument.presentationml.presentation"},
            {".rar", "application/vnd.rar"},
            {".rtf", "application/rtf"},
            {".sh", "application/x-sh"},
            {".svg", "image/svg+xml"},
            {".tar", "application/x-tar"},
            {".tiff", "image/tiff"},
            {".tif", "image/tiff"},
            {".ttf", "font/ttf"},
            {".txt", "text/plain"},
            {".wav", "audio/wav"},
            {".weba", "audio/webm"},
            {".webm", "video/webm"},
            {".webp", "image/webp"},
            {".woff", "font/woff"},
            {".woff2", "font/woff2"},
            {".xhtml", "application/xhtml+xml"},
            {".xls", "application/vnd.ms-excel"},
            {".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
            {".xml", "application/xml"},
            {".xml", "text/xml"},
            {".zip", "application/zip"},
            {".7z", "application/x-7z-compressed"}
            
        };

        string getMimeFromExtension(string filepath) {

            // Save the files extension.
            string extension = filepath.substr(filepath.find("."), 
                                               filepath[filepath.length() - 1]);

            // Create an iterator which looks for the extension.
            map<string, string>::iterator it = MIMETYPE.find(extension);
            if(it != MIMETYPE.end()) {
                // Key,Value pair was found!
                currentMIMEType = MIMETYPE[extension];
                return currentMIMEType;
            } else {
                spdlog::get("http_client_logger")
                    ->error("The given MIMEType is currently not supported!");

                throw logic_error("This MIME Type is currently not supported!");
            }
        }
};



#endif