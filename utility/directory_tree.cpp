#include <queue>
#include "directory_tree.hpp"


/* DirectoryTree methods */
DirectoryTree::DirectoryTree()
{
}

DirectoryTree::~DirectoryTree()
{
}


// TODO: Check that function can handle /home and /home/
// It currently assumes all directory paths will have a trailing / (i.e. /home/)
void DirectoryTree::AddNode(int watch_descriptor, boost::filesystem::path directory_path)
{
    DirectoryTree::Node *node, *parent_node;
    std::string directory_path_string = directory_path.string();
    
    // Check that path is absolute
    if (!directory_path.is_absolute()) {
        fprintf(stderr, "Path given to AddNode must be absolute: %s\n", 
                directory_path_string.c_str());
        exit(EXIT_FAILURE);
    }

    // Allocate new node if it doesn't exist
    if (path_to_node_.count(directory_path_string) != 0) {
        fprintf(stderr, "Adding a node that already exists\n");
        exit(EXIT_FAILURE);
    }
    node = new DirectoryTree::Node(watch_descriptor, directory_path);
     
    // Check if parent directory exists in tree
    std::string parent_path = directory_path.parent_path().string();
    auto search = path_to_node_.find(parent_path);
    if (search != path_to_node_.end()) { // parent exists
        parent_node = search->second;
        node->SetParent(parent_node);
        parent_node->AddChild(node);
    } 

    // Insert node into maps
    std::pair<std::map<std::string,DirectoryTree::Node*>::iterator,bool> path_ret;
    std::pair<std::string,DirectoryTree::Node*> str_pair(directory_path_string, node);
    path_ret = path_to_node_.insert(str_pair);
    if (path_ret.second == false) {
        fprintf(stderr, "Attempting to add a node that already exists into path_to_node_\n");
        exit(EXIT_FAILURE);
    }

    std::pair<std::map<int,DirectoryTree::Node*>::iterator,bool> int_ret;
    std::pair<int,DirectoryTree::Node*> int_pair(watch_descriptor, node);
    int_ret = wd_to_node_.insert(int_pair);
    if (int_ret.second == false) {
        fprintf(stderr, "Attempting to add a node that already exists into wd_to_node_\n");
        exit(EXIT_FAILURE);
    }
}

// TODO: Handle exception
void DirectoryTree::RemoveNode(boost::filesystem::path directory_path)
{
    DirectoryTree::Node *node; 
    std::string directory_path_string = directory_path.string();
    
    node = path_to_node_.at(directory_path_string);
    if (node->NumChildren() > 0) {
        fprintf(stderr, "Deleting node with children\n");
        exit(EXIT_FAILURE);
    }

    // Unlink from parent if one exists
    if (node->HasParent()) {
        node->parent_->RemoveChild(node);
        node->UnsetParent();
    }
    
    // Remove from maps
    path_to_node_.erase(directory_path_string);
    wd_to_node_.erase(node->watch_descriptor_);

    // Free memory for node
    delete node;
}


void DirectoryTree::RemoveNode(int watch_descriptor)
{
    DirectoryTree::Node *node; 
    
    node = wd_to_node_.at(watch_descriptor);
    if (node->NumChildren() > 0) {
        fprintf(stderr, "Deleting node with children\n");
        exit(EXIT_FAILURE);
    }

    // Unlink from parent if one exists
    if (node->HasParent()) {
        node->parent_->RemoveChild(node);
        node->UnsetParent();
    }
    
    // Remove from maps
    path_to_node_.erase(node->directory_path_.string());
    wd_to_node_.erase(node->watch_descriptor_);

    // Free memory for node
    delete node;
}

boost::filesystem::path DirectoryTree::GetPath(int watch_descriptor)
{
    DirectoryTree::Node *node;

    node = wd_to_node_.at(watch_descriptor);
    return node->directory_path_;
}

void DirectoryTree::SetModifyBit(int watch_descriptor)
{
    DirectoryTree::Node *node = wd_to_node_.at(watch_descriptor); 
    
    while (node != NULL) {
        node->modified_ = true;
        node = node->parent_;
    }
}

void DirectoryTree::ResetModifyBit(boost::filesystem::path directory_path)
{
    std::queue<DirectoryTree::Node*> q;
    DirectoryTree::Node *node;
    std::string directory_path_string = directory_path.string();

    node = path_to_node_.at(directory_path_string);
    q.push(node);

    while (!q.empty()) {
        node = q.front();
        q.pop();

        node->modified_ = false;
        for (std::list<Node*>::iterator it=node->children_.begin(); it != node->children_.end();
                ++it) {
            q.push(*it);
        }
    }
}

enum UPDATE_FLAG DirectoryTree::GetUpdateFlag(boost::filesystem::path directory_path)
{
    DirectoryTree::Node *node;
    std::map<std::string, DirectoryTree::Node*>::iterator it;
    enum UPDATE_FLAG flag;
    std::string directory_path_string = directory_path.string();

    it = path_to_node_.find(directory_path_string);
    // If node with that watch descriptor doesn't exist
    if (it == path_to_node_.end()) {
        flag = DOESNOTEXIST;
        return flag;
    }

    node = it->second;
    if (node->modified_) {
        flag = MODIFIED;
    } else {
        flag = NOCHANGE;
    }

    return flag;
}

DirectoryTree::Node::Node(int watch_descriptor, boost::filesystem::path directory_path)
{
    watch_descriptor_ = watch_descriptor;
    directory_path_ = directory_path;
    parent_ = NULL;
    modified_ = false;
}

DirectoryTree::Node::~Node()
{
}
