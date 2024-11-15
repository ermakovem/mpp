#include <iostream>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <random>
#include <array>
#include <chrono>
#include <set>


class Node {
public:
    Node(int val_, Node* left_, Node* right_) : val(val_), left(left_), right(right_) {} 
    int val;
    Node* left;
    Node* right;
};

class BST_printer { //stole this code to print my bst for check purporses 
public:
    Node* root;
    void print(Node* n) {
        root = n;
        const int d = get_max_depth(root);
    
        // If this tree is empty, tell someone
        if(d == 0) {
            std::cout << " <empty tree>\n";
            return;
        }

        // This tree is not empty, so get a list of node values...
        const auto rows_disp = get_row_display();
        // then format these into a text representation...
        auto formatted_rows = row_formatter(rows_disp);
        // then trim excess space characters from the left sides of the text...
        trim_rows_left(formatted_rows);
        // then dump the text to cout.
        for(const auto& row : formatted_rows) {
            std::cout << ' ' << row << '\n';
        }
    }
private:
    int get_max_depth(Node* node) const {

        if (node == nullptr) {
            return 0;
        }

        int leftDepth = get_max_depth(node->left);
        int rightDepth = get_max_depth(node->right);
        
        return std::max(leftDepth, rightDepth) + 1;
    }

        struct cell_display {
        std::string valstr;
        bool present;
        cell_display() : present(false) {}
        cell_display(std::string valstr) : valstr(valstr), present(true) {}
    };

    using display_rows = std::vector< std::vector< cell_display > >;

    // The text tree generation code below is all iterative, to avoid stack faults.

    // get_row_display builds a vector of vectors of cell_display structs
    // each vector of cell_display structs represents one row, starting at the root
    display_rows get_row_display() const {
        // start off by traversing the tree to
        // build a vector of vectors of Node pointers
        std::vector<Node*> traversal_stack;
        std::vector< std::vector<Node*> > rows;
        if(!root) return display_rows();
    
        Node *p = root;
        const int max_depth = get_max_depth(root);
        rows.resize(max_depth);
        int depth = 0;
        for(;;) {
            // Max-depth Nodes are always a leaf or null
            // This special case blocks deeper traversal
            if(depth == max_depth-1) {
                rows[depth].push_back(p);
                if(depth == 0) break;
                --depth;
                continue;
            }

            // First visit to node?  Go to left child.
            if(traversal_stack.size() == depth) {
                rows[depth].push_back(p);
                traversal_stack.push_back(p);
                if(p) p = p->left;
                ++depth;
                continue;
            }
        
            // Odd child count? Go to right child.
            if(rows[depth+1].size() % 2) {
                p = traversal_stack.back();
                if(p) p = p->right;
                ++depth;
                continue;
            }

            // Time to leave if we get here

            // Exit loop if this is the root
            if(depth == 0) break;

            traversal_stack.pop_back();
            p = traversal_stack.back();
            --depth;
        }

        // Use rows of Node pointers to populate rows of cell_display structs.
        // All possible slots in the tree get a cell_display struct,
        // so if there is no actual Node at a struct's location,
        // its boolean "present" field is set to false.
        // The struct also contains a string representation of
        // its Node's value, created using a std::stringstream object.
        display_rows rows_disp;
        std::stringstream ss;
        for(const auto& row : rows) {
            rows_disp.emplace_back();
            for(Node* pn : row) {
                if(pn) {
                    ss << pn->val;
                    rows_disp.back().push_back(cell_display(ss.str()));
                    ss = std::stringstream();
                } else {
                    rows_disp.back().push_back(cell_display());
        }   }   }
        return rows_disp;
    }

    // row_formatter takes the vector of rows of cell_display structs 
    // generated by get_row_display and formats it into a test representation
    // as a vector of strings
    std::vector<std::string> row_formatter(const display_rows& rows_disp) const {
        using s_t = std::string::size_type;

        // First find the maximum value string length and put it in cell_width
        s_t cell_width = 0;
        for(const auto& row_disp : rows_disp) {
            for(const auto& cd : row_disp) {
                if(cd.present && cd.valstr.length() > cell_width) {
                    cell_width = cd.valstr.length();
        }   }   }

        // make sure the cell_width is an odd number
        if(cell_width % 2 == 0) ++cell_width;

        // allows leaf nodes to be connected when they are
        // all with size of a single character
        if(cell_width < 3) cell_width = 3;


        // formatted_rows will hold the results
        std::vector<std::string> formatted_rows;
    
        // some of these counting variables are related,
        // so its should be possible to eliminate some of them.
        s_t row_count = rows_disp.size();

        // this row's element count, a power of two
        s_t row_elem_count = 1 << (row_count-1);

        // left_pad holds the number of space charactes at the beginning of the bottom row
        s_t left_pad = 0;
    
        // Work from the level of maximum depth, up to the root
        // ("formatted_rows" will need to be reversed when done) 
        for(s_t r=0; r<row_count; ++r) {
            const auto& cd_row = rows_disp[row_count-r-1]; // r reverse-indexes the row
            // "space" will be the number of rows of slashes needed to get
            // from this row to the next.  It is also used to determine other
            // text offsets.
            s_t space = (s_t(1) << r) * (cell_width + 1) / 2 - 1;
            // "row" holds the line of text currently being assembled
            std::string row;
            // iterate over each element in this row
            for(s_t c=0; c<row_elem_count; ++c) {
                // add padding, more when this is not the leftmost element
                row += std::string(c ? left_pad*2+1 : left_pad, ' ');
                if(cd_row[c].present) {
                    // This position corresponds to an existing Node
                    const std::string& valstr = cd_row[c].valstr;
                    // Try to pad the left and right sides of the value string
                    // with the same number of spaces.  If padding requires an
                    // odd number of spaces, right-sided children get the longer
                    // padding on the right side, while left-sided children
                    // get it on the left side.
                    s_t long_padding = cell_width - valstr.length();
                    s_t short_padding = long_padding / 2;
                    long_padding -= short_padding;
                    row += std::string(c%2 ? short_padding : long_padding, ' ');
                    row += valstr;
                    row += std::string(c%2 ? long_padding : short_padding, ' ');
                } else {
                    // This position is empty, Nodeless...
                    row += std::string(cell_width, ' ');
                }
            }
            // A row of spaced-apart value strings is ready, add it to the result vector
            formatted_rows.push_back(row);
        
            // The root has been added, so this loop is finsished
            if(row_elem_count == 1) break;

            // Add rows of forward- and back- slash characters, spaced apart
            // to "connect" two rows' Node value strings.
            // The "space" variable counts the number of rows needed here.
            s_t left_space  = space + 1;
            s_t right_space = space - 1;
            for(s_t sr=0; sr<space; ++sr) {
                std::string row;
                for(s_t c=0; c<row_elem_count; ++c) {
                    if(c % 2 == 0) {
                        row += std::string(c ? left_space*2 + 1 : left_space, ' ');
                        row += cd_row[c].present ? '/' : ' ';
                        row += std::string(right_space + 1, ' ');
                    } else {
                        row += std::string(right_space, ' ');
                        row += cd_row[c].present ? '\\' : ' ';
                    }
                }
                formatted_rows.push_back(row);
                ++left_space;
                --right_space;
            }
            left_pad += space + 1;
            row_elem_count /= 2;
        }

        // Reverse the result, placing the root node at the beginning (top)
        std::reverse(formatted_rows.begin(), formatted_rows.end());
    
        return formatted_rows;
    }

    // Trims an equal number of space characters from
    // the beginning of each string in the vector.
    // At least one string in the vector will end up beginning
    // with no space characters.
    static void trim_rows_left(std::vector<std::string>& rows) {
        if(!rows.size()) return;
        auto min_space = rows.front().length();
        for(const auto& row : rows) {
            auto i = row.find_first_not_of(' ');
            if(i==std::string::npos) i = row.length();
            if(i == 0) return;
            if(i < min_space) min_space = i;
        }
        for(auto& row : rows) {
            row.erase(0, min_space);
    }   }
};


class BST {
private:
    //using Window = std::array<Node*, 3>;
    Node* root;
public:
    BST() : root(nullptr) {}

    Node* get_root() {
        return root;
    }

    bool insert(int i) {
        if (root == nullptr) {
            root = new Node(i, nullptr, nullptr);
            return true;
        }

        std::array<Node*, 3> n = search(i);
        if (i == n[0]->val) {
            return false;
        } else {
            insert_helper(n[0], i);
            return true;
        }
    }

    bool remove(int i) {
        if (root == nullptr) {
            return false;
        }

        std::array<Node*, 3> n = search(i);
        if (i != n[0]->val) {
            return false;
        } else {
            remove_helper(n);
            return true;
        }
    }

    bool contains(int i) const {
        if (root == nullptr) {
            return false;
        }

        if (i == search(i)[0]->val) {
            return true;
        } else {
            return false;
        }
    }

private:
    void insert_helper(Node* n, int i) {
        Node* clone_node = new Node(n->val, nullptr, nullptr);
        Node* new_node = new Node(i, nullptr, nullptr);
        if (i > n->val) {
            n->right = new_node;
            n->left = clone_node;
        } else {
            n->right = clone_node;
            n->left = new_node;
        }
    }

    void remove_helper(std::array<Node*, 3> n) {
        if (n[2] == nullptr && n[1] == nullptr) {
            //   (root)
            //    /  \   =>   root = nullptr
            //
            // only root left

            if (n[0] != root) { //sanity check
                throw std::runtime_error("was expecting to delete root but got smthing else");
            }

            delete root;
            root = nullptr;

        } else if (n[2] == nullptr && n[1] != nullptr) {
            //   [root]
            //    /  \ 
            //  (1)  (2)    =>   (new_root)
            //  /\    /\            /  \ 
            // only root and 2 children left

            if (n[1] != root) { //sanity check
                throw std::runtime_error("was expecting to delete root but got smthing else");
            } 
            if (n[1]->left == n[0]) {
                root = n[1]->right;
            } else {
                root = n[1]->left;
            }
            delete n[1];
            delete n[0];
        } else {
            //    [0]              [0]
            //    /  \             /  \ 
            //  (0)  [1]    =>   (0)   \ 
            //       / \                \ 
            //     (1) (2)              (2)
            // any other combination
            Node* to_connect = nullptr;
            if (n[1]->left == n[0]) {
                to_connect = n[1]->right;
            } else {
                to_connect = n[1]->left;
            }

            if (n[2]->left == n[1]) {
                n[2]->left = to_connect;
            } else {
                n[2]->right = to_connect;
            }
            
            delete n[1];
            delete n[0];
        }

    }

    std::array<Node*, 3> search(int i) const {
        Node* cursor = root;
        Node* gprev = nullptr;
        Node* prev = nullptr;
        while (!is_leaf(cursor)) {
            //std::cout << cursor->val << '\n';
            gprev = prev;
            prev = cursor;
            if (i > cursor->val) {
                cursor = cursor->right;
            } else if (i < cursor->val) {
                cursor = cursor->left;
            } else if (i == cursor->val) {
                if (cursor->left->val == i) {
                    cursor = cursor->left;
                } else {
                    cursor = cursor->right;
                }
                //return cursor?
            }
        }
        return {cursor, prev, gprev};
    }

    //or is_external
    bool is_leaf(Node* n) const {
        if (n->left == nullptr && n->right == nullptr) {
            return true;
        } else {
            return false;
        }
    }

    bool is_valid() const {
        return false;
    }
};

std::vector<int> generate_vector(int n) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(-1 * n, n);
    std::vector<int> v;
    for (int i = 0; i < n; i++) {
        v.push_back(dis(gen));
    }
    return v;
}
#define TEST_SIZE 10'000'000
int main() {

    std::vector<int> v = generate_vector(TEST_SIZE);
    BST bst = BST();

    auto start = std::chrono::high_resolution_clock::now();

    for (int i : v) {
        bst.insert(i);
    }
    for (int i : v) {
        bst.remove(i);
    }

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Время выполнения для моей реализации: " << elapsed.count() << " секунд" << std::endl;

    std::set<int> s;

    start = std::chrono::high_resolution_clock::now();
    
    for (int i : v) {
        s.insert(i);
    }
    for (int i : v) {
        s.erase(i);
    }

    end = std::chrono::high_resolution_clock::now();

    elapsed = end - start;
    std::cout << "Время выполнения для set: " << elapsed.count() << " секунд" << std::endl;

    Node* n = bst.get_root();
    BST_printer p = BST_printer();
    p.print(bst.get_root());
    //std::cout << ' ' << n->val << std::endl << n->left->val << "  " <<  n->right->val << std::endl;
}