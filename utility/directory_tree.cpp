#include "directory_tree.hpp"
#include <queue>

// TODO: Handle exceptions for all node_map_.at(watch_descriptor) calls


/* DirectoryTree methods */
DirectoryTree::DirectoryTree()
{
}

DirectoryTree::~DirectoryTree()
{
}

void DirectoryTree::InsertNode(int watch_descriptor, boost::filesystem::path directory_path, int parent_watch_descriptor)
{
    DirectoryTree::Node *node, *parent;

    if (parent_watch_descriptor > -1) {
        parent = node_map_.at(parent_watch_descriptor);
    }

    node = new DirectoryTree::Node(watch_descriptor, directory_path);
    node_map_[watch_descriptor] = node;

    if (parent_watch_descriptor > -1) {
        parent->AddChild(node);
        node->SetParent(parent);
    }
}


void DirectoryTree::RemoveNode(int watch_descriptor)
{
    DirectoryTree::Node *node; 
    boost::filesystem::path path;
    
    // TODO: This will throw an exception if the element is not found
    node = node_map_.at(watch_descriptor);
    if (node->NumChildren() > 0) {
        fprintf(stderr, "Deleting node with children\n");
        exit(EXIT_FAILURE);
    }

    if (node->parent_ != NULL) {
        node->parent_->RemoveChild(node);
        node->parent_ = NULL;
    }
    
    node_map_.erase(watch_descriptor);
    delete node;
}


void DirectoryTree::SetModifyBit(int watch_descriptor)
{
    DirectoryTree::Node *node = node_map_.at(watch_descriptor);
    
    while (node != NULL) {
        node->modified_ = true;
        node = node->parent_;
    }
}


enum UPDATE_FLAG DirectoryTree::GetUpdateFlag(int watch_descriptor)
{
    DirectoryTree::Node *node;
    std::map<int, DirectoryTree::Node*>::iterator it;
    enum UPDATE_FLAG flag;

    it = node_map_.find(watch_descriptor);
    // If node with that watch descriptor doesn't exist
    if (it == node_map_.end()) {
        flag = DELETED;
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


void DirectoryTree::ResetModifyBit(int watch_descriptor)
{
    std::queue<DirectoryTree::Node*> q;
    DirectoryTree::Node *node;

    node = node_map_.at(watch_descriptor);
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


boost::filesystem::path DirectoryTree::GetPath(int watch_descriptor)
{
    DirectoryTree::Node *node = node_map_.at(watch_descriptor);
    return node->directory_path_;
}


/* DirectoryTree::Node methods */
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
