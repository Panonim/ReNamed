#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <regex.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>

/* Program constants */
#define MAX_PATH 1024
#define MAX_FILES 1000
#define VERSION "a.2"
#define REPO_URL "https://github.com/Panonim/ReNamed"
#define DEFAULT_LOG_FILE "renamed_log.txt"
#define MAX_PATTERN_LENGTH 256

/* File entry structure to store file information */
typedef struct {
    char original_name[MAX_PATH];
    char new_name[MAX_PATH];
    int episode_number;
    int is_special;
} FileEntry;

/* Global configuration */
typedef struct {
    int force_mode;      /* Force renaming of all file types */
    int keep_originals;  /* Keep original files (create backups) */
    int dry_run;         /* Dry run mode - don't actually rename files */
    int use_log;         /* Create log file */
    int use_custom_pattern; /* Use custom regex pattern */
    char output_path[MAX_PATH]; /* Custom output path */
    char log_file[MAX_PATH];    /* Log file path */
    char custom_pattern[MAX_PATTERN_LENGTH]; /* Custom regex pattern */
} ProgramConfig;

/* Get file extension from filename */
const char *get_file_extension(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot;
}

/* Check if the file is a special episode based on filename patterns */
int is_special_episode(const char *filename) {
    regex_t regex;
    int result = 0;
    
    /* Patterns to identify special episodes */
    const char *patterns[] = {
        "[Ss]pecial",
        "SP[0-9]+",
        "OVA",
        "Extra",
        "Bonus"
    };
    
    for (int i = 0; i < sizeof(patterns) / sizeof(patterns[0]); i++) {
        if (regcomp(&regex, patterns[i], REG_EXTENDED | REG_ICASE) != 0) {
            continue;
        }
        
        if (regexec(&regex, filename, 0, NULL, 0) == 0) {
            result = 1;
            regfree(&regex);
            break;
        }
        regfree(&regex);
    }
    
    return result;
}

/* Extract episode number using a custom pattern */
int extract_episode_number_custom(const char *filename, const char *pattern) {
    regex_t regex;
    regmatch_t matches[3]; /* Up to 2 capture groups + the full match */
    char episode_str[10] = {0};
    int episode_num = 0;

    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        printf("Error compiling custom pattern: %s\n", pattern);
        return 0;
    }

    if (regexec(&regex, filename, 3, matches, 0) == 0) {
        /* If we have two capture groups, assume it's Season-Episode format */
        if (matches[2].rm_so != -1) {
            int length = matches[2].rm_eo - matches[2].rm_so;
            if (length < sizeof(episode_str)) {
                strncpy(episode_str, filename + matches[2].rm_so, length);
                episode_str[length] = '\0';
                episode_num = atoi(episode_str);
            }
        } 
        /* Otherwise use the first capture group */
        else if (matches[1].rm_so != -1) {
            int length = matches[1].rm_eo - matches[1].rm_so;
            if (length < sizeof(episode_str)) {
                strncpy(episode_str, filename + matches[1].rm_so, length);
                episode_str[length] = '\0';
                episode_num = atoi(episode_str);
            }
        }
    }

    regfree(&regex);
    return episode_num;
}

/* Extract episode number from various filename formats */
int extract_episode_number(const char *filename) {
    regex_t regex;
    regmatch_t matches[2];
    char episode_str[10] = {0};

    /* Common episode number patterns */
    const char *patterns[] = {
        "Episode[ ]*([0-9]{1,3})",         /* Episode 1, Episode 12 */
        "Ep[ ]*([0-9]{1,3})",              /* Ep 1, Ep12 */
        "E([0-9]{1,3})([^0-9]|$)",         /* E01, E12 */
        "-[ ]*([0-9]{1,3})([^0-9]|$)",     /* - 01, -12 */
        "S[0-9]+[ ]*-[ ]*([0-9]{1,3})",    /* S2 - 10 */
        "S[0-9]+[ ]+([0-9]{1,3})",         /* S2 08 */
        "SP[ ]*([0-9]{1,3})",              /* SP01, SP 3 (for specials) */
        " ([0-9]{1,2})[^0-9]"              /* Fallback: isolated numbers */
    };

    /* Try each pattern until we find a match */
    for (int i = 0; i < sizeof(patterns) / sizeof(patterns[0]); i++) {
        if (regcomp(&regex, patterns[i], REG_EXTENDED) != 0) {
            continue;
        }

        if (regexec(&regex, filename, 2, matches, 0) == 0) {
            int length = matches[1].rm_eo - matches[1].rm_so;
            if (length < sizeof(episode_str)) {
                strncpy(episode_str, filename + matches[1].rm_so, length);
                episode_str[length] = '\0';
                regfree(&regex);

                /* Pad single digits with leading zero */
                if (strlen(episode_str) == 1) {
                    char padded_episode_str[3] = {'0', episode_str[0], '\0'};
                    return atoi(padded_episode_str);
                }
                return atoi(episode_str);
            }
        }
        regfree(&regex);
    }
    
    /* Fallback: look for isolated 2-digit numbers */
    for (size_t i = 0; i < strlen(filename) - 1; i++) {
        if (isdigit(filename[i]) && isdigit(filename[i + 1])) {
            /* Make sure it's not part of a larger number */
            if ((i == 0 || !isdigit(filename[i - 1])) &&
                (i + 2 >= strlen(filename) || !isdigit(filename[i + 2]))) {
                char num[3] = {filename[i], filename[i + 1], '\0'};
                return atoi(num);
            }
        }
    }
    return 0; /* No episode number found */
}

/* Create directory if it doesn't exist */
int create_directory(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        #ifdef _WIN32
        if (mkdir(path) != 0) {
            printf("Error creating directory '%s': %s\n", path, strerror(errno));
            return 0;
        }
        #else
        if (mkdir(path, 0755) != 0) {
            printf("Error creating directory '%s': %s\n", path, strerror(errno));
            return 0;
        }
        #endif
        return 1;
    }
    return 1; /* Directory already exists */
}

/* Copy a file from source to destination */
int copy_file(const char *source, const char *destination) {
    FILE *src, *dst;
    char buffer[4096];
    size_t bytes_read;
    
    src = fopen(source, "rb");
    if (!src) {
        printf("Error opening source file '%s': %s\n", source, strerror(errno));
        return 0;
    }
    
    dst = fopen(destination, "wb");
    if (!dst) {
        printf("Error opening destination file '%s': %s\n", destination, strerror(errno));
        fclose(src);
        return 0;
    }
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        if (fwrite(buffer, 1, bytes_read, dst) != bytes_read) {
            printf("Error writing to destination file '%s': %s\n", destination, strerror(errno));
            fclose(src);
            fclose(dst);
            return 0;
        }
    }
    
    fclose(src);
    fclose(dst);
    return 1;
}

/* Log operation to file */
void log_operation(FILE *log_file, const char *action, const char *old_path, const char *new_path, int success) {
    time_t now;
    struct tm *timeinfo;
    char timestamp[20];
    
    time(&now);
    timeinfo = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    fprintf(log_file, "[%s] %s: %s -> %s [%s]\n", 
            timestamp, 
            action, 
            old_path, 
            new_path, 
            success ? "SUCCESS" : "FAILED");
}

/* Compare function for sorting files */
int compare_files(const void *a, const void *b) {
    FileEntry *fileA = (FileEntry *)a;
    FileEntry *fileB = (FileEntry *)b;
    
    /* Group regular episodes before specials */
    if (fileA->is_special != fileB->is_special) {
        return fileA->is_special - fileB->is_special;
    }
    
    /* Sort by episode number */
    return fileA->episode_number - fileB->episode_number;
}

/* Display version information */
void print_version() {
    printf("ReNamed - Automatic Episode Renamer v%s\n", VERSION);
    printf("Repository: %s\n", REPO_URL);
    printf("\nA simple tool to rename and organize TV show episodes.\n");
}

/* Display usage information */
void print_usage(char *program_name) {
    printf("Usage: %s [OPTIONS]\n\n", program_name);
    printf("Options:\n");
    printf("  -v           Display version information\n");
    printf("  -h           Display this help message\n");
    printf("  -f           Force renaming of all file types (not just video files)\n");
    printf("  -k           Keep original files\n");
    printf("  -d           Dry run mode (only show what would happen, don't rename files)\n");
    printf("  -p <path>    Specify custom output path for renamed files\n");
    printf("  --log[=file] Create log file (default: renamed_log.txt)\n");
    printf("  --pattern=<regex> Specify custom regex pattern for episode detection\n");
    printf("               Example: --pattern='Season (\\d+)-Episode (\\d+)'\n\n");
    printf("If no options are provided, the program runs in interactive mode.\n");
}

/* Check if a file is a video file */
int is_video_file(const char *extension) {
    return (strcasecmp(extension, ".mp4") == 0 ||
            strcasecmp(extension, ".mkv") == 0 ||
            strcasecmp(extension, ".avi") == 0);
}

int main(int argc, char *argv[]) {
    ProgramConfig config = {0}; /* Initialize config with defaults */
    int opt;
    int option_index = 0;
    
    /* Default log file name */
    strcpy(config.log_file, DEFAULT_LOG_FILE);

    /* Define long options */
    static struct option long_options[] = {
        {"log",     optional_argument, 0,  'l' },
        {"pattern", required_argument, 0,  'r' },
        {0,         0,                 0,  0   }
    };

    /* Use getopt for command line parsing */
    while ((opt = getopt_long(argc, argv, "vhfkdp:", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'v':
                print_version();
                return 0;
            case 'h':
                print_usage(argv[0]);
                return 0;
            case 'f':
                config.force_mode = 1;
                break;
            case 'k':
                config.keep_originals = 1;
                break;
            case 'd':
                config.dry_run = 1;
                break;
            case 'p':
                strncpy(config.output_path, optarg, MAX_PATH - 1);
                config.output_path[MAX_PATH - 1] = '\0';
                break;
            case 'l': /* --log option */
                config.use_log = 1;
                if (optarg) {
                    strncpy(config.log_file, optarg, MAX_PATH - 1);
                    config.log_file[MAX_PATH - 1] = '\0';
                }
                break;
            case 'r': /* --pattern option */
                config.use_custom_pattern = 1;
                strncpy(config.custom_pattern, optarg, MAX_PATTERN_LENGTH - 1);
                config.custom_pattern[MAX_PATTERN_LENGTH - 1] = '\0';
                break;
            default:
                printf("Unknown option: %c\n", opt);
                print_usage(argv[0]);
                return 1;
        }
    }

    /* Parse non-option arguments for --log and --pattern */
    for (int i = optind; i < argc; i++) {
        if (strncmp(argv[i], "--log", 5) == 0) {
            config.use_log = 1;
            char *equals = strchr(argv[i], '=');
            if (equals && *(equals + 1)) {
                strncpy(config.log_file, equals + 1, MAX_PATH - 1);
                config.log_file[MAX_PATH - 1] = '\0';
            }
        } else if (strncmp(argv[i], "--pattern=", 10) == 0) {
            config.use_custom_pattern = 1;
            strncpy(config.custom_pattern, argv[i] + 10, MAX_PATTERN_LENGTH - 1);
            config.custom_pattern[MAX_PATTERN_LENGTH - 1] = '\0';
        }
    }

    char show_name[MAX_PATH];
    char folder_path[MAX_PATH];
    char destination_path[MAX_PATH] = {0};
    char specials_path[MAX_PATH];
    char confirm[10];
    FileEntry files[MAX_FILES];
    int file_count = 0;
    DIR *dir;
    struct dirent *entry;
    FILE *log_fp = NULL;

    /* Open log file if logging is enabled */
    if (config.use_log) {
        log_fp = fopen(config.log_file, "a");
        if (!log_fp) {
            printf("Warning: Could not open log file '%s': %s\n", config.log_file, strerror(errno));
            printf("Continuing without logging.\n");
            config.use_log = 0;
        } else {
            /* Write header to log file */
            time_t now;
            struct tm *timeinfo;
            char timestamp[20];
            
            time(&now);
            timeinfo = localtime(&now);
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
            
            fprintf(log_fp, "\n----- ReNamed Session Started at %s -----\n", timestamp);
            if (config.dry_run) {
                fprintf(log_fp, "[INFO] Running in DRY RUN mode - no actual changes made\n");
            }
        }
    }

    /* Get show name from user */
    printf("Enter show name: ");
    if (fgets(show_name, sizeof(show_name), stdin) == NULL) {
        printf("Error reading input.\n");
        if (log_fp) fclose(log_fp);
        return 1;
    }
    show_name[strcspn(show_name, "\n")] = 0; /* Remove newline */
    
    if (strlen(show_name) == 0) {
        printf("Show name cannot be empty.\n");
        if (log_fp) fclose(log_fp);
        return 1;
    }

    /* Get folder path from user */
    printf("Enter folder path with source files: ");
    if (fgets(folder_path, sizeof(folder_path), stdin) == NULL) {
        printf("Error reading input.\n");
        if (log_fp) fclose(log_fp);
        return 1;
    }
    folder_path[strcspn(folder_path, "\n")] = 0; /* Remove newline */
    
    if (strlen(folder_path) == 0) {
        printf("Folder path cannot be empty.\n");
        if (log_fp) fclose(log_fp);
        return 1;
    }

    /* If output path not specified via command line, use source path or ask user */
    if (strlen(config.output_path) == 0) {
        if (config.keep_originals) {
            /* Ask for destination path when keeping originals */
            printf("Enter destination folder path for renamed files: ");
            if (fgets(destination_path, sizeof(destination_path), stdin) == NULL) {
                printf("Error reading input.\n");
                if (log_fp) fclose(log_fp);
                return 1;
            }
            destination_path[strcspn(destination_path, "\n")] = 0; /* Remove newline */
            
            if (strlen(destination_path) == 0) {
                printf("Destination path cannot be empty when using backup mode.\n");
                if (log_fp) fclose(log_fp);
                return 1;
            }
        } else {
            /* Use source folder as destination (for in-place renaming) */
            strncpy(destination_path, folder_path, MAX_PATH - 1);
            destination_path[MAX_PATH - 1] = '\0';
        }
    } else {
        /* Use path provided from command line */
        strncpy(destination_path, config.output_path, MAX_PATH - 1);
        destination_path[MAX_PATH - 1] = '\0';
    }

    /* Log operation details */
    if (log_fp) {
        fprintf(log_fp, "[INFO] Show name: '%s'\n", show_name);
        fprintf(log_fp, "[INFO] Source folder: '%s'\n", folder_path);
        fprintf(log_fp, "[INFO] Destination folder: '%s'\n", destination_path);
        if (config.use_custom_pattern) {
            fprintf(log_fp, "[INFO] Using custom pattern: '%s'\n", config.custom_pattern);
        }
    }

    /* Create destination directory if it doesn't exist (even in dry run, for planning) */
    if (!config.dry_run && !create_directory(destination_path)) {
        printf("Error: Failed to create destination directory '%s'\n", destination_path);
        if (log_fp) fclose(log_fp);
        return 1;
    }

    /* Create path for specials directory */
    snprintf(specials_path, sizeof(specials_path), "%s/Specials", destination_path);

    /* Try to open directory */
    dir = opendir(folder_path);
    if (dir == NULL) {
        printf("Error: Unable to open directory '%s': %s\n", folder_path, strerror(errno));
        if (log_fp) fclose(log_fp);
        return 1;
    }

    /* Scan directory for files */
    if (config.force_mode) {
        printf("Scanning directory for all files (force mode)...\n");
    } else {
        printf("Scanning directory for video files...\n");
    }
    
    while ((entry = readdir(dir)) != NULL && file_count < MAX_FILES) {
        /* Skip . and .. directories */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        /* Skip directories */
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", folder_path, entry->d_name);

        struct stat path_stat;
        if (stat(full_path, &path_stat) != 0) {
            printf("Warning: Cannot get stats for '%s': %s\n", entry->d_name, strerror(errno));
            continue;
        }
        
        if (!S_ISREG(path_stat.st_mode))
            continue;

        /* Get file extension */
        const char *extension = get_file_extension(entry->d_name);

        /* Skip non-video files unless force mode is enabled */
        if (!config.force_mode && !is_video_file(extension))
            continue;

        /* Check if this is a special episode */
        int special = is_special_episode(entry->d_name);
        
        /* Extract episode number using either custom or default patterns */
        int episode_num;
        if (config.use_custom_pattern) {
            episode_num = extract_episode_number_custom(entry->d_name, config.custom_pattern);
        } else {
            episode_num = extract_episode_number(entry->d_name);
        }

        if (episode_num == 0) {
            printf("Warning: No episode number found in '%s', skipping.\n", entry->d_name);
            if (log_fp) {
                fprintf(log_fp, "[WARNING] No episode number found in '%s', skipping.\n", entry->d_name);
            }
            continue;
        }

        /* Store file information */
        strcpy(files[file_count].original_name, entry->d_name);
        files[file_count].episode_number = episode_num;
        files[file_count].is_special = special;

        /* Generate new filename based on type */
        if (special) {
            snprintf(files[file_count].new_name, MAX_PATH, "%s - %02d - Special%s",
                    show_name, episode_num, extension);
        } else {
            snprintf(files[file_count].new_name, MAX_PATH, "%s - %02d%s",
                    show_name, episode_num, extension);
        }

        file_count++;
    }

    closedir(dir);

    if (file_count == 0) {
        printf("No suitable files found in the directory.\n");
        if (log_fp) {
            fprintf(log_fp, "[INFO] No suitable files found in the directory.\n");
            fclose(log_fp);
        }
        return 1;
    }

    /* Sort files by episode number */
    qsort(files, file_count, sizeof(FileEntry), compare_files);

    /* Display the rename plan */
    printf("\nFound %d files. Rename Plan%s:\n", file_count, config.dry_run ? " (DRY RUN)" : "");
    
    /* Show operation mode */
    if (config.dry_run) {
        printf("Operation mode: DRY RUN - no actual changes will be made\n");
    } else if (config.keep_originals) {
        printf("Operation mode: Copying files (keeping originals)\n");
    } else {
        printf("Operation mode: Moving/renaming files\n");
    }
    printf("Destination directory: %s\n", destination_path);
    if (config.use_log) {
        printf("Logging enabled: '%s'\n", config.log_file);
    }
    if (config.use_custom_pattern) {
        printf("Using custom pattern: '%s'\n", config.custom_pattern);
    }
    
    printf("\n%-70s -> %s\n", "Original Filename", "New Filename");
    printf("--------------------------------------------------------------------------------\n");

    /* Check if any special episodes exist */
    int has_special_episodes = 0;
    for (int i = 0; i < file_count; i++) {
        if (files[i].is_special) {
            has_special_episodes = 1;
            break;
        }
    }

    for (int i = 0; i < file_count; i++) {
        char orig_truncated[71] = {0};
        strncpy(orig_truncated, files[i].original_name, 70);
        if (strlen(files[i].original_name) > 70) {
            strcpy(orig_truncated + 67, "...");
        }
        
        if (files[i].is_special) {
            printf("%-70s -> Specials/%s (SPECIAL)\n", orig_truncated, files[i].new_name);
        } else {
            printf("%-70s -> %s\n", orig_truncated, files[i].new_name);
        }
    }

    /* Skip confirmation in dry run mode */
    if (config.dry_run) {
        printf("\nDRY RUN completed. No files were modified.\n");
        if (log_fp) {
            fprintf(log_fp, "[INFO] DRY RUN completed. No files were modified.\n");
            fprintf(log_fp, "----- ReNamed Session Ended -----\n\n");
            fclose(log_fp);
        }
        return 0;
    }

    /* Ask for confirmation */
    printf("\nContinue with %s? (yes/no): ", config.keep_originals ? "copying" : "renaming");
    if (fgets(confirm, sizeof(confirm), stdin) == NULL) {
        printf("Error reading input.\n");
        if (log_fp) {
            fprintf(log_fp, "[ERROR] Failed to read user confirmation.\n");
            fclose(log_fp);
        }
        return 1;
    }

    if (strncasecmp(confirm, "yes", 3) == 0 || strncasecmp(confirm, "y", 1) == 0) {
        /* Create destination directory if different from source */
        if (strcmp(folder_path, destination_path) != 0) {
            if (!create_directory(destination_path)) {
                printf("Error: Failed to create destination directory '%s'\n", destination_path);
                if (log_fp) {
                    fprintf(log_fp, "[ERROR] Failed to create destination directory '%s'\n", destination_path);
                    fclose(log_fp);
                }
                return 1;
            }
        }
    
        /* Create specials directory only if special episodes exist */
        if (has_special_episodes) {
            if (create_directory(specials_path)) {
                printf("Created 'Specials' directory in '%s'.\n", destination_path);
                if (log_fp) {
                    fprintf(log_fp, "[INFO] Created 'Specials' directory in '%s'.\n", destination_path);
                }
            }
        }
        
        /* Perform renaming/copying */
        int success_count = 0;
        int special_count = 0;
        int regular_count = 0;

        for (int i = 0; i < file_count; i++) {
            char old_path[MAX_PATH];
            char new_path[MAX_PATH];

            snprintf(old_path, sizeof(old_path), "%s/%s", folder_path, files[i].original_name);
            
            if (files[i].is_special) {
                snprintf(new_path, sizeof(new_path), "%s/%s", specials_path, files[i].new_name);
                special_count++;
            } else {
                snprintf(new_path, sizeof(new_path), "%s/%s", destination_path, files[i].new_name);
                regular_count++;
            }

            if (config.keep_originals) {
                /* Copy the file instead of renaming */
                int success = copy_file(old_path, new_path);
                if (success) {
                    success_count++;
                    printf("Copied '%s' to '%s'\n", files[i].original_name, new_path);
                    if (log_fp) {
                        log_operation(log_fp, "COPY", old_path, new_path, 1);
                    }
                } else {
                    printf("Error copying '%s' to '%s'\n", files[i].original_name, new_path);
                    if (log_fp) {
                        log_operation(log_fp, "COPY", old_path, new_path, 0);
                    }
                }
            } else {
                /* Rename/move the file */
                if (rename(old_path, new_path) == 0) {
                    success_count++;
                    printf("Renamed '%s' to '%s'\n", files[i].original_name, files[i].new_name);
                    if (log_fp) {
                        log_operation(log_fp, "RENAME", old_path, new_path, 1);
                    }
                } else {
                    printf("Error renaming '%s' to '%s': %s\n", 
                          files[i].original_name, 
                          files[i].new_name,
                          strerror(errno));
                    if (log_fp) {
                        log_operation(log_fp, "RENAME", old_path, new_path, 0);
                    }
                }
            }
        }

        printf("\nOperation complete!\n");
        printf("- %d of %d files successfully %s\n", 
               success_count, file_count, 
               config.keep_originals ? "copied" : "renamed");
        printf("- %d regular episodes\n", regular_count);
        printf("- %d special episodes", special_count);
        if (special_count > 0) {
            printf(" moved to Specials folder");
        }
        printf("\n");
        
        if (log_fp) {
            fprintf(log_fp, "[INFO] Operation complete! %d of %d files successfully %s.\n", 
                   success_count, file_count, config.keep_originals ? "copied" : "renamed");
            fprintf(log_fp, "[INFO] %d regular episodes, %d special episodes.\n", 
                   regular_count, special_count);
            fprintf(log_fp, "----- ReNamed Session Ended -----\n\n");
        }
    } else {
        printf("Operation cancelled.\n");
        if (log_fp) {
            fprintf(log_fp, "[INFO] Operation cancelled by user.\n");
            fprintf(log_fp, "----- ReNamed Session Ended -----\n\n");
        }
    }

    if (log_fp) {
        fclose(log_fp);
    }

    return 0;
}
