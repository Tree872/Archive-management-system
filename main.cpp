#include <stdio.h> 
#include <string.h>
#include <stdint.h>
#include <vector>
#include <string>
#include "Archive.h"
#include "Tree.h"

void printMenu(const char* currentArchive);
void promptString(const char* prompt, char* input, int sizeOfBuffer);
void promptInt(const char* prompt, int* input);
int isInteger(const char* str);
void flushInputStream();

int main() {
  char currentArchive[256] = "None";
  PathNode* root = createNode("ROOT", false);
	int entriesAdded = 0;
	FILE* archivePtr = NULL;

	while (1) {
    char archiveName[256]; // For user input
    int choice;
    printMenu(currentArchive);
    promptInt("Enter your choice (1-7): ", &choice);
		switch (choice) {
    case 1: // Create a new archive
      archiveName[0] = '\0'; 
			promptString("Enter archive name (without extension): ", archiveName, sizeof(archiveName));
			if (strlen(archiveName) == 0) {
				printf("Archive name cannot be empty.\n");
				break;
			}
      char archiveFullName[256];
      sprintf_s(archiveFullName, sizeof(archiveFullName), "%s.dsp", archiveName);
			if (fileOrDirectoryExists(archiveFullName)) {
				printf("An archive with this name already exists. Please choose a different name.\n");
				break;
			}
			createArchive(archiveName);
			freeTree(root); // Free existing tree if any
			root = createNode("ROOT", false); // Reset tree
			printf("Archive '%s.dsp' created successfully.\n", archiveName);
			break;

    case 2: // Open an existing archive
			archiveName[0] = '\0';
			promptString("Enter archive name to open (without extension): ", archiveName, sizeof(archiveName));
			if (strlen(archiveName) == 0) {
				printf("Archive name cannot be empty.\n");
				break;
			}
			archivePtr = openArchive(archiveName);
			if (archivePtr != NULL) {
				freeTree(root); // Free existing tree if any
				root = createNode("ROOT", false); // Reset tree
				printf("Archive '%s.dsp' opened successfully.\n", archiveName);
				strcpy_s(currentArchive, sizeof(currentArchive), archiveName);
			}
			break;

    case 3: // Add a file to the archive
			if (strcmp(currentArchive, "None") == 0) {
				printf("No archive is currently open. Please create or open an archive first.\n");
				break;
			}
			char filePath[256];
			promptString("Enter file path to add: ", filePath, sizeof(filePath));
			if (strlen(filePath) == 0) {
				printf("File path cannot be empty.\n");
				break;
			}
			if (!fileOrDirectoryExists(filePath) || !isFile(filePath)) {
				printf("The specified file does not exist.\n");
				break;
			}
			addFile(archivePtr, filePath, root);
      entriesAdded++;
			printf("File '%s' added to archive '%s.dsp'.\n", filePath, currentArchive);
			break;

    case 4: // Add a directory to the archive
			if (strcmp(currentArchive, "None") == 0) {
				printf("No archive is currently open. Please create or open an archive first.\n");
				break;
			}
			char dirPath[256];
			promptString("Enter directory path to add: ", dirPath, sizeof(dirPath) - 1);
			if (strlen(dirPath) == 0) {
				printf("Directory path cannot be empty.\n");
				break;
			}
			if (!fileOrDirectoryExists(dirPath) || !isDirectory(dirPath)) {
				printf("The specified directory does not exist.\n");
				break;
			}
      printf("Adding directory '%s' to archive '%s.dsp'...\n", dirPath, currentArchive);
			entriesAdded = addDirectory(archivePtr, dirPath, root);
			printf("Directory '%s' added to archive '%s.dsp' with %d entries.\n", dirPath, currentArchive, entriesAdded);
			break;

    case 5: // Unpack the archive
			if (strcmp(currentArchive, "None") == 0) {
				printf("No archive is currently open. Please create or open an archive first.\n");
				break;
			}
      printf("Unpacking archive '%s.dsp'...\n", currentArchive);
			unpackArchive(archivePtr, currentArchive);
			printf("Archive '%s.dsp' unpacked to directory '%s'.\n", currentArchive, currentArchive);
			break;

    case 6: // Display the contents of the archive
			if (strcmp(currentArchive, "None") == 0) {
				printf("No archive is currently open. Please create or open an archive first.\n");
				break;
			}
			if (entriesAdded == 0) {
				printf("No entries in the archive '%s.dsp'.\n", currentArchive);
				break;
      }
			printf("Contents of archive '%s.dsp':\n", currentArchive);
			printTree(root);
			break;

    case 7: // Exit the program
			if (strcmp(currentArchive, "None") != 0) {
				closeArchive(archivePtr);
			}
			printf("Exiting program.\n");
      freeTree(root); 
			return 0;
    default:
      printf("Invalid choice. Please enter a number between 1 and 7.\n");
      break;
		}
	}

  freeTree(root);
  return 0;
}
// FUNCTION: printMenu
// DESCRIPTION:
//		Prints the main menu with the current archive name.
// PARAMETERS:
//		const char* currentArchive : The name of the currently opened archive.
// RETURNS:
//		void
void printMenu(const char* currentArchive) {
  printf("\nCurrentArchive: %s\n", currentArchive);
  printf("1. Create Archive\n");
  printf("2. Open Archive\n");
  printf("3. Add File to Archive\n");
  printf("4. Add Directory to Archive\n");
  printf("5. Unpack Archive\n");
  printf("6. Display Archive Contents\n");
  printf("7. Exit\n");
}

// FUNCTION: promptString
// DESCRIPTION:
//		Prompts the user for a string input and validates it.
//		Continues to prompt until a valid string (smaller than the given buffer) is entered.
// PARAMETERS:
//		const char* prompt : The prompt message to display to the user.
//		char* input : Pointer to the character array where the input will be stored.
// RETURNS:
//		void
void promptString(const char* prompt, char* input, int sizeOfBuffer) {
	while (1) {
		printf("%s", prompt);
		fgets(input, sizeOfBuffer + 1, stdin); // Writes directly into input
		// When user input exceeds buffer amount
		if (!strchr(input, '\n')) {
			flushInputStream();
			printf("Input exceeds character limits. Please try again or press enter to skip this field.\n");
		}
		else if (strlen(input) == 1) { // User pressed enter without input
			input[0] = '\0'; // Set to empty string
			return;
		}
		else { // Valid input
			input[strlen(input) - 1] = '\0'; // Remove the trailing newline  
			return;
		}
	}
}
// FUNCTION: promptInt
// DESCRIPTION:
//		Prompts the user for an integer input and validates it.
//		Continues to prompt until a valid integer is entered.
// PARAMETERS:
//		const char* prompt : The prompt message to display to the user.
//		int* input : Pointer to the integer where the input will be stored.
// RETURNS:
//    void
void promptInt(const char* prompt, int* input) {
	char inputBuffer[100];
	while (1) {
		printf("%s", prompt);
		fgets(inputBuffer, sizeof(inputBuffer), stdin);
		// When user input exceeds buffer amount
		if (!strchr(inputBuffer, '\n')) {
			flushInputStream();
			printf("Input exceeds buffer size. Please try again.\n");
			continue;
		}

		inputBuffer[strlen(inputBuffer) - 1] = '\0'; // Remove the trailing newline 

		if (isInteger(inputBuffer)) {
			sscanf_s(inputBuffer, "%d", input); // Writes directly into input
			return;
		}
		else {
			printf("Input must be a valid integer. Try again\n");
		}
	}
}
// FUNCTION: isInteger
// DESCRIPTION:
//		Checks if the given string is a valid integer.
// PARAMETERS:
//		const char *str : Pointer to the string to be checked.
// RETURNS:
//		int : 1 if the string is a valid integer, 0 otherwise.
int isInteger(const char* str) {
	char extra;
	int value;
	if (sscanf_s(str, "%d %c", &value, &extra) != 1) {
		return 0;
	}
	else {
		return 1;
	}
}
// FUNCTION: flushInputStream
// DESCRIPTION:
//		Flushes the input stream to remove any remaining characters.
// PARAMETERS:
//		void
// RETURNS:
//		void
void flushInputStream() {
	char extra = ' ';
	while (extra != '\n' && extra != EOF) {
		extra = getchar();
	}
}