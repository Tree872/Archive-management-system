#include <Windows.h>
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
  PathNode* root = createNode("root", false);

  

  printTree(root);
  freeTree(root);
  return 0;
}

void printMenu(const char* currentArchive) {
  printf("\nCurrentArchive: %s\n", currentArchive);
  printf("1. Create Archive\n");
  printf("2. Open Archive\n");
  printf("3. Add File to Archive\n");
  printf("4. Add Directory to Archive\n");
  printf("5. Unpack Archive\n");
  printf("6. Exit\n");
}

// FUNCTION: promptString
// DESCRIPTION:
//		Prompts the user for a string input and validates it.
//		Continues to prompt until a valid string (smaller than the given buffer) is entered.
//		Allows the user to skip the input by pressing enter.
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