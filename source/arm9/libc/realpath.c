// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2024 Antonio Niño Díaz

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *realpath(const char *restrict path, char *restrict resolved_path)
{
    // FAT doesn't support symbolic links, so all we need to convert relative
    // paths into absolute paths and handle ".", "..", and redundant '/'
    // characters.

    // TODO: We should check that all directories between the root and the last
    // component exist (and set errno to ENOENT if the directory doesn't exist,
    // or ENOTDIR if the component isn't a directory), but this would involve a
    // lot of calls to stat(), so leave that out for now.

    if (path == NULL)
    {
        errno = EINVAL;
        return NULL;
    }

    if (strlen(path) == 0)
    {
        errno = ENOENT;
        return NULL;
    }

    if (resolved_path == NULL)
    {
        // If the user didn't provide a destination buffer allocate it here and
        // call realpath() recursively. If it fails, free the buffer before
        // returning.
        char *buffer = malloc(PATH_MAX);
        if (buffer == NULL)
        {
            errno = ENOMEM;
            return NULL;
        }

        char *result = realpath(path, buffer);
        if (result == NULL)
            free(buffer);

        return result;
    }

    // Clear the destination buffer
    resolved_path[0] = '\0';

    // First, check if the path is absolute or relative. If it's relative it
    // will be needed to get the current working directory.

    bool is_relative_path = false;

    const char *first_slash = strchr(path, '/');
    if (first_slash)
    {
        size_t index = first_slash - path;
        if (index == 0)
        {
            // This is an absolute path without drive name. Add the drive name.

            char *cwd = getcwd(resolved_path, PATH_MAX);
            if (cwd == NULL)
                return NULL; // If getcwd() fails, rely on the errno set inside

            // Look for the end of the drive name
            char *p = strstr(resolved_path, ":/");
            if (!p)
            {
                // This should never happen, getcwd() should always return the
                // drive name.
                errno = EINVAL;
                return NULL;
            }

            // Keep only the drive name
            p[2] = '\0';

            // Skip initial "/" of the user-provided path to turn path into a
            // relative path from the root directory.
            path++;
        }
        else if (path[index - 1] == ':')
        {
            if (index + 1 >= PATH_MAX)
            {
                errno = ENAMETOOLONG;
                return NULL;
            }

            // This is an absolute path with drive name. Copy the drive name.
            strncpy(resolved_path, path, index + 1);
            resolved_path[index + 1] = '\0';

            // Skip drive name of the user-provided path up to right after the
            // initial "/" to turn the path into a relative path from the root
            // directory.
            path += index + 1;
        }
        else
        {
            // This is a relative path with slashes.
            is_relative_path = true;
        }
    }
    else
    {
        // This is a relative path without slashes (just a file/directory name)
        is_relative_path = true;
    }

    if (is_relative_path)
    {
        // This is a relative path. Get the current working directory,
        // which includes the drive name.

        char *cwd = getcwd(resolved_path, PATH_MAX);
        if (cwd == NULL)
            return NULL; // If getcwd() fails, rely on the errno set inside

        // Make sure that the path ends in "/"
        size_t len = strlen(resolved_path);
        if (resolved_path[len - 1] != '/')
        {
            if (len >= PATH_MAX)
            {
                errno = ENOMEM;
                return NULL;
            }
            strcat(resolved_path, "/");
        }

        // We don't need to modify the user-provided path
    }

    // Append the rest of the path, handling "." and ".." and redundant "/".

    while (1)
    {
        if (path[0] == '\0')
            break;

        const char *next_name;
        size_t name_len;

        // Are there more slashes in the remaining string?
        char *slash = strchr(path, '/');
        if (slash == NULL)
        {
            // The name of the component is the remaining string
            name_len = strlen(path);
            next_name = path + name_len; // Point to the terminator character
        }
        else
        {
            // The name of the component ends in the next slash
            name_len = slash - path;
            next_name = slash + 1; // Point to the character after the slash
        }

        // TODO: In theory, we need to check if the lenght of each component is
        // greater than NAME_MAX and set errno to ENAMETOOLONG if so. However,
        // this restriction is pretty arbitrary, and we would need to check the
        // directories in the path returned by getcwd() as well, so it is
        // ignored in this implementation of realpath().

        if (name_len == 0)
        {
            // Redundant "/" characters: "example//path"
            path = next_name;
            continue;
        }
        else if (name_len == 1)
        {
            // Check if this is a ".". If not, it's a regular name.
            if (path[0] == '.')
            {
                path = next_name;
                continue;
            }
        }
        else if (name_len == 2)
        {
            // Check if this is a "..". If not, it's a regular name.
            if ((path[0] == '.') && (path[1] == '.'))
            {
                // Go back until a slash is found. If it isn't found, we're
                // already in the root directory. That's not an error, stay in
                // the root directory.

                int i = strlen(resolved_path) - 2;
                while (i >= 0)
                {
                    if (resolved_path[i] == '/')
                    {
                        resolved_path[i + 1] = 0;
                        break;
                    }
                    i--;
                }

                path = next_name;
                continue;
            }
        }

        // Regular name, copy it as it is.

        // Check if the name fits (current path + new component + '/' + '\0'
        if (strlen(resolved_path) + name_len + 1 + 1 > PATH_MAX)
        {
            errno = ENAMETOOLONG;
            return NULL;
        }

        strncat(resolved_path, path, name_len);
        strcat(resolved_path, "/");

        path = next_name;
    }

    // If the path ends with "/", remove it (except if it's part of the drive
    // name). "nitro:/" is the correct path, "nitro:/folder" is also correct,
    // but "nitro:/folder/" isn't.
    //
    // At this point we can be sure that the path at least contains the drive
    // name, so we don't need to check if this string is long enough for the
    // "- 1" and "- 2".
    size_t len = strlen(resolved_path);
    if (resolved_path[len - 1] == '/')
    {
        if (resolved_path[len - 2] != ':')
        {
            resolved_path[len - 1] = '\0';
        }
    }

    return resolved_path;
}
