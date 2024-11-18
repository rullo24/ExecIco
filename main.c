// std C includes
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// windows includes
#include <windows.h>
#include <errno.h>
#include <shlwapi.h>

void print_help() {
	printf("Usage: ./execico <*.ico file name*>\n");
}

bool check_filepath_exists(char *filepath) {
	DWORD file_attr = GetFileAttributesA(filepath);
    if (file_attr == INVALID_FILE_ATTRIBUTES) {
        fprintf(stderr, "ERROR: %s\n", "filepath DOES NOT exist");
        return false;
    }
    return true;
}

// PRESUMPTION: file exists in computer --> check already done
bool is_absolute_path(const char *filepath, size_t filepath_size) {
	// size check to avoid buffer overflow
	if (filepath_size < 3) { 
		return false; // do not print an error as relative filename could be small maybe?
	}

    // checking if absolute path
    if (filepath[1] == ':' && isalpha((unsigned char)filepath[0]) && filepath[2] == '\\') { // Check if the path starts with a drive letter (e.g., "C:\" or "D:\")
        return true; // Absolute path with drive letter
    } else if (filepath[0] == '\\' && filepath[1] == '\\') { // Check if the path starts with "\\" for UNC paths
        return true; // UNC path
    } else {
	    return false;
    } 
}

// returns 0 for success || returns < 0 for failure
int get_exe_dir(char *path_buf, size_t path_buf_size) {
	DWORD get_file_res = GetModuleFileNameA(NULL, path_buf, path_buf_size); // path pushed to path_buf
	if (get_file_res == 0) {
		fprintf(stderr, "ERROR: %s\n", "failed to capture the exe's abs location");
		return -1;
	}
	
	// removing the filename from the exe path
	BOOL rm_filename_from_exe_loc_res = PathRemoveFileSpec(path_buf); // removing filename from abs exe location
	if (!rm_filename_from_exe_loc_res) {
		fprintf(stderr, "ERROR %s\n", "failed to get abs dir from abs exe location");
		return -2;
	}

	return 0;
}

// safely concatenating a windows filename to a directory path
int join_filename_to_dir_path(const char *filename, char *full_path_buf, char *dir_buf) {
	if (!PathCombineA(full_path_buf, dir_buf, filename)) {
		fprintf(stderr, "ERROR: %s\n", "failed to combine paths using the windows API");
		return -1;
	}

	return 0;
}

int change_all_backslashes_to_forwardslashes(char *path_buf, size_t path_buf_size) {
	size_t num_increments = 1; // used for buffer overflow check
	char *temp_path_ptr = path_buf;

	// checking for end of string
	while (*temp_path_ptr != '\0') {
		
		// turning backslash into forward slash
		if (*temp_path_ptr == '\\') {
			*temp_path_ptr = '/';
		}
		
		// checking if can increment (buffer overflow checking)
		if ((path_buf_size == num_increments)) { // last valid char in string reached (excl NULL byte)
		 	if (temp_path_ptr[path_buf_size] != '\0') { // checking if the string is max buffer size and valid --> if not valid, throw error
				fprintf(stderr, "ERROR %s\n", "preventing buffer overflow (change_all_backslashes_to_forwardslashes)");
				return -1;
			}
		}
		temp_path_ptr++; // move to next letter in the ptr
	}

	return 0;
}

int main(int argc, char *argv[]) {
	// only runs if one argument parsed with program
	if (argc < 2 || argc > 2) { 
		print_help();
		return -1;
	}
	char *filepath = argv[1]; // variable name given for simplicity
	size_t filepath_size = strlen(filepath); // as this is parsed by windows, null byte presumed at end of string

	// checking that the parsed filepath actually exists
	bool filepath_exists_res = check_filepath_exists(filepath);
	if (!filepath_exists_res) { // could not find actual file
		return -1;
	}

	// checking if the parsed filename is absolute or relative
	bool is_abs_path = is_absolute_path(filepath, filepath_size);

	char full_path_buf[MAX_PATH]; // will hold the absolute path of .ico file
	char exe_dir_buf[MAX_PATH]; // will hold the current exe's path (if applicable) --> defined in main scope to avoid going out of scope
	if (!is_abs_path) {
		// getting directory of the main executable
		int exe_dir_res = get_exe_dir(exe_dir_buf, sizeof(exe_dir_buf));
		if (exe_dir_res < 0) {
			return -1;
		}

		// getting the absolute path for the filename
		int full_path_res = join_filename_to_dir_path(filepath, full_path_buf, exe_dir_buf);
		if (full_path_res < 0) {
			return -1;
		}

		// checking that the newly created path exists
		bool full_path_exists = check_filepath_exists(full_path_buf);
		if (!full_path_exists) {
			fprintf(stderr, "ERROR %s\n", "concatenated path DOES NOT exist");
			return -1;
		}

	} else {
		strncpy(full_path_buf, filepath, filepath_size); // copying absolute filepath (already given) to the char*
	}

	// getting .rc file absolute location and checking if it exists
	char rc_file_loc[MAX_PATH];
	int rc_path_res = join_filename_to_dir_path("icon.rc", rc_file_loc, exe_dir_buf);
	if (rc_path_res < 0) {
		fprintf(stderr, "ERROR: %s\n", "failed to join resource.rc to abs path");
		return -1;
	}
	
	// opening .rc file for writing .ico filename to before obj file compilation
	FILE *p_rc_file = fopen(rc_file_loc, "w");
	if (p_rc_file == NULL) {
		fprintf(stderr, "ERROR %s\n", "could not open resource.rc file for writing");
		return -1;
	}

	// writing out the formatted resource.rc file
	char rc_info_buf[512];
	snprintf(rc_info_buf, sizeof(rc_info_buf), "#include <windows.h>\n\nIDR_MAINICON ICON \"%s\"", full_path_buf);
	int back_change_res = change_all_backslashes_to_forwardslashes(rc_info_buf, sizeof(rc_info_buf));
	if (back_change_res < 0) {
		return -1;
	}

	// write string to file synchronously so that the program halts until writing is complete
    if (fputs(rc_info_buf, p_rc_file) == EOF) {
		return -1;
	} 
	fclose(p_rc_file); // closing the file pointer
	p_rc_file = NULL; // nullifying to avoid dangling pointer

	// checking that the file was created --> data presumed written to said file
	if (!check_filepath_exists(rc_file_loc)) {
		fprintf(stderr, "ERROR: %s\n", "did not create .rc file successfully");
		return -1;
	}

	// using windres program that is part of MinGW to compile an object file from the .ico
	char *rc_create_command = "windres icon.rc -o icon.o";
	int cmd_res = system(rc_create_command);
	if (cmd_res != 0) {
		fprintf(stderr, "ERROR %s\n", "failed to run the command");
		return -1;
	}

	// successful run
	printf("SUCCESS: %s\n", "icon.o created");
	return 0;
}
