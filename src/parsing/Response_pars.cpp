#include "../../includes/Response.hpp"
#include "../../includes/Request.hpp"
#include "../../includes/parsing.hpp"
#include <ftw.h>
#include <iostream>
#include <sys/stat.h>

#include <dirent.h>
#include <unistd.h>

std::string Request::handle_autoindex(const std::string& directoryPath) {
    DIR *dir;
    struct dirent *entry;
    std::string html = "<html>\n<head><title>Index of " + directoryPath + "</title></head>\n<body>\n<h1>Index of " + directoryPath + "</h1>\n<hr>\n<pre>\n";

    dir = opendir(directoryPath.c_str());
    if (!dir) {
        status_value = 500;
        return "";
    }

    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;

        // Ignore the "." and ".." directories
        if (name == "." || name == "..")
            continue;

        html += "<a href=\"" + name + "\">" + name + "</a>\n";
    }

    closedir(dir);

    html += "</pre>\n<hr>\n</body>\n</html>\n";
    return html;
}

void Request::handle_get(Config &config, Location location)
{
    (void) config;
    struct stat sb;

    size_t size;
    if(location.getLocationPath() == "/")
        size = 0;
    else
        size = location.getLocationPath().size();
    std::string targetPath = location.getRoot() + getPath().substr(size);
    std::cout << targetPath <<std::endl;
    if (stat(targetPath.c_str(), &sb) == 0) {
        if (S_ISDIR(sb.st_mode)) {
            if (getPath()[getPath().size() - 1] != '/' && location.getLocationPath() != "/") {
                status_value = 301;
                fullpath = getPath() + '/';
                return;
            }
            else if (location.getAutoindex() == "on")
            {
                fullpath = targetPath;  // For auto-indexing
                a_body = handle_autoindex(targetPath);
                status_value = 1;  // For auto-indexing
                return;
            }
            else {
                fullpath = targetPath + "/" + location.getIndex();
                if (access(fullpath.c_str(), F_OK) == -1)
                    status_value = 403;  // No index file and autoindex is off
                else
                    status_value = 200;
            }
        } else {
            fullpath = targetPath;
            status_value = (access(fullpath.c_str(), F_OK) != -1) ? 200 : 404;
        }
    } else {
        status_value = 404;
    }
}


void Request::check_request(std::vector<Config>& parsing)
{
	(void)parsing;
    std::vector<Location> locations = _host.getLocations();
    std::vector<Location>::iterator it;

    for (it = locations.begin(); it != locations.end(); ++it) {
        Location location = *it;
        if (getPath().find(location.getLocationPath()) != std::string::npos)
        {
            /*if(it->getRedirect().find("301"))
            {
                fullpath = "gag";
                status_value = 301;
                return;
            }*/
            break;
        }
    }
    if(it == locations.end())
    {
        if(locations[0].getLocationPath()== "/")
        {
            it = locations.begin();
            std::cout << it->getLocationPath()<< std::endl;
        }
        else
            status_value = 404;
    }
    std::cout << status_value<< std::endl;
    if(status_value > 0)
        return;
    if(getMethod() == "GET")
    {
        
        handle_get(_host,*it);
    }
    else if(getMethod() == "POST")
    {
        handle_post(_host,*it);
    }
    else if(getMethod() == "DELETE")
    {

        handle_delete(_host,*it);
    }
    // std::cout << status_value << std::endl;
}

int delete_file(const char* fpath, const struct stat* sb, int typeflag, struct FTW* ftwbuf)
{
    (void) sb;
    (void) typeflag;
    (void) ftwbuf;
    if (remove(fpath) == 0) {
        std::cout << "Deleted file: " << fpath << std::endl;
        return 0;
    } else {
        std::cerr << "Failed to delete file: " << fpath << std::endl;
        return -1;
    }
}



int delete_directory_recursive(const std::string& directoryPath)
{
    int flags = FTW_DEPTH | FTW_PHYS;

    int result = nftw(directoryPath.c_str(), delete_file, 20, flags);
    if (result == -1) {
        return 500;
    }

    // Delete the directory itself
    if (rmdir(directoryPath.c_str()) == 0) {
        return 204;
    } else {
        return 500;
    }
}

void Request::handle_delete(Config &config,Location location)
{
    (void) config;
    (void) location;
    struct stat sb;
    
   size_t size;
    if(location.getLocationPath() == "/")
        size = 0;
    else
        size = location.getLocationPath().size();
    std::string targetPath = location.getRoot() + getPath().substr(size);
    if (stat(targetPath.c_str(), &sb) == 0) {
        if (S_ISDIR(sb.st_mode)) {
            if (getPath()[getPath().size() - 1] != '/' && location.getLocationPath() != "/") {
                status_value = 409;
                return;
            }
            else {
                status_value = delete_directory_recursive(targetPath.c_str());
            }
        } else {
            fullpath = targetPath;
                 if (remove(targetPath.c_str()) == 0)
                    status_value =  204;
                else
                    status_value =  403;
        }
    } else {
        status_value = 404;
    }
}
void Request::handle_post(Config &config,Location location)
{
    (void) config;
    (void) location;

    if(!location.getUpload().empty()) // la kan fih upload sf rah good  201
    {
        status_value = 201;
        fullpath = location.getUpload() + '/';
        return;
    }
    
    struct stat sb;

    size_t size;
    if(location.getLocationPath() == "/")
        size = 0;
    else
        size = location.getLocationPath().size();
    std::string targetPath = location.getRoot() + getPath().substr(size);

    if (stat(targetPath.c_str(), &sb) == 0) {
        if (S_ISDIR(sb.st_mode)) {
            if (getPath()[getPath().size() - 1] != '/') {
                status_value = 301;
                fullpath = getPath() + '/';
                return;
            }
            else {
                fullpath = targetPath + "/" + location.getIndex();
                if (access(fullpath.c_str(), F_OK) == -1)
                    status_value = 403;  // No index file and autoindex is off
                else
                {
                    if(location.getCgiPath().empty())
                        status_value = 403;
                    else
                         status_value = 201; // Cgi location
                }
            }
        } else {
            fullpath = targetPath;
             if(location.getCgiPath().empty())
                status_value = 403;
            else
                 status_value = 201;
        }
    } else {
        status_value = 404;
    }
}