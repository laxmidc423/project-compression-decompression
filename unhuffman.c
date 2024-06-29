#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#ifdef _WIN32
#include <direct.h>  // For _mkdir on Windows
#else
#include <sys/stat.h>  // For mkdir on POSIX systems
#endif

// Structure used to define a node
typedef struct node_t {
    struct node_t *left, *right;
    int freq;
    char c;
} *node;

// Global variables
int n_nodes = 0, qend = 1;  // Global variables for keeping track of number of nodes and end of the queue
struct node_t pool[256] = {{0}};  // Pool of nodes
node qqq[255], *q = qqq - 1;  // The priority queue

// Function to create a new node
node new_node(int freq, char c, node a, node b) {
    node n = pool + n_nodes++;
    if (freq != 0) {
        n->c = c;  // Assign the character 'c' to the character of the node (eventually a leaf)
        n->freq = freq;  // Assign frequency
    } else {
        n->left = a, n->right = b;  // If there is no frequency provided with the invoking
        n->freq = a->freq + b->freq;  // The removed nodes at the end of the queue will be added to left and right
    }
    return n;
}

// Function to insert a node into the priority queue
void qinsert(node n) {
    int j, i = qend++;
    while ((j = i / 2)) {
        if (q[j]->freq <= n->freq) break;
        q[i] = q[j], i = j;
    }
    q[i] = n;
}

node qremove() {
    int i, l;
    node n = q[i = 1];

    if (qend < 2) return 0;
    qend--;
    while ((l = i * 2) < qend) {
        if (l + 1 < qend && q[l + 1]->freq < q[l]->freq) l++;
        q[i] = q[l], i = l;
    }
    q[i] = q[qend];
    return n;  // Return the node
}

void import_table(FILE *fp_table, unsigned int *freq) {
    char c;  // Temporary variable
    int i = 0;

    while ((c = fgetc(fp_table)) != EOF) {
        freq[i++] = (unsigned char)c;  // Get the values from the .table file to update the Huffman tree
    }
    for (i = 0; i < 128; i++) {
        if (freq[i]) {
            printf("Char %c: Frequency %d\n", i, freq[i]);  // Print for verification
            qinsert(new_node(freq[i], i, 0, 0));  // Insert new nodes into the queue if there is a frequency
        }
    }
    while (qend > 2)
        qinsert(new_node(0, 0, qremove(), qremove()));  // Build the tree
}

void decode(FILE *fp_huffman, FILE *fp_out) {
    int i = 0, lim = 0, j = 0;
    char c;
    node n = q[1];

    fscanf(fp_huffman, "%d", &lim);  // Get the length of the bit stream from the header
    fseek(fp_huffman, 1, SEEK_CUR);  // Seek one position to avoid the new line character of the header

    printf("Decoded: \n");
    for (i = 0; i < lim; i++) {
        if (j == 0)
            c = fgetc(fp_huffman);

        if (c & (1 << (7 - j)))  // Check the j-th bit from the left (MSB first)
            n = n->right;        // Go right
        else
            n = n->left;         // Go left

        if (n->c) {  // If a leaf (node with a character) is reached
            putchar(n->c);         // Print the character to console for debugging
            fputc(n->c, fp_out);   // Write the character to the output file
            n = q[1];              // Reset to the root of the Huffman tree
        }

        if (++j > 7) j = 0;  // Move to the next byte
    }

    putchar('\n');  // End of the decoded output line
    if (q[1] != n) printf("garbage input\n");  // Check for any remaining bits not processed
}

int main(int argc, char* argv[]) {
    FILE *fp_table, *fp_huffman, *fp_out;  // FIFO pointers
    char file_name[50] = {0}, temp[50] = {0};  // File name
    unsigned int freq[128] = {0};  // Frequency of the letters

    if (argc == 2) {
        strcpy(file_name, argv[1]);  // Command-line argument directly allows to compress the file
        if (strstr(file_name, "huffman") == NULL) {
            printf("\nERROR: wrong file format!\n");
            return 0;
        }
    } else if (argc > 2) {
        printf("Too many arguments supplied.\n");
        return 0;
    } else {
        printf("Please enter the file to be compressed: ");  // Else a prompt comes to enter the file name
        scanf("%s", file_name);
        if (strstr(file_name, "huffman") == NULL) {
            printf("\nERROR: wrong file format!\n");
            return 0;
        }
    }

    // Import the frequency table and compressed data from the file and create the Huffman tree
    if ((fp_huffman = fopen(file_name, "r")) == NULL) {
        printf("\nERROR: No such file\n");
        return 0;
    }
    strcat(file_name, ".table");  // File pointer for .table file
    if ((fp_table = fopen(file_name, "r")) == NULL) {  // Open the file stream
        printf("\nERROR: Frequency table cannot be found\n");
        fclose(fp_huffman);
        return 0;
    }
    import_table(fp_table, freq);  // Import the file and fill the frequency array

    *strstr(file_name, ".huffman") = '\0';  // Search for .huffman and remove it
    strcpy(temp, file_name);  // Concatenating strings for the command
    strcat(temp, ".decoded");

    // Create directory if it doesn't exist
#ifdef _WIN32
    if (_mkdir(temp) && errno != EEXIST) {
#else
    if (mkdir(temp, 0777) && errno != EEXIST) {
#endif
        printf("ERROR: Creating directory failed\n");
        fclose(fp_huffman);
        fclose(fp_table);
        return 0;
    }

    strcpy(temp, file_name);  // Reset temp to the base file name
    strcat(temp, ".decoded/");
    if ((fp_out = fopen(strcat(temp, file_name), "w")) == NULL) {
        printf("ERROR: Creating decoded file failed\n");  // If any error occurred during creating file, exit
        fclose(fp_huffman);
        fclose(fp_table);
        return 0;
    }

    
    decode(fp_huffman, fp_out);  // Decode the file and save

    fclose(fp_huffman);  // Close the file pointer for .huffman file
    fclose(fp_table);  // Close the file pointer for Huffman frequency table file
    fclose(fp_out);  // Close the file pointer for output file
    return 0;
}
