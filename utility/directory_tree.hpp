#ifndef DIRECTORY_TREE_H
#define DIRECTORY_TREE_H

#include <string>
#include <list>
#include <map>
#include "boost/filesystem.hpp"

enum UPDATE_FLAG { MODIFIED, DELETED, NOCHANGE };


class DirectoryTree {
    public:
        DirectoryTree();
        ~DirectoryTree();
        // Create a new node with watch_descriptor and directory_path, and insert
        // the newly created node into the tree with it's parent set to the node
        // which has parent_watch_descriptor. If parent_watch_descriptor is -1, the
        // node has no parent.
        void InsertNode(int watch_descriptor, boost::filesystem::path directory_path,
                int parent_watch_descriptor = -1);

        // Remove the node from the tree. If the node has children, an error will
        // occur. After removing the node from the tree, the memory for the node
        // is deallocated with a call to delete.
        void RemoveNode(int watch_descriptor);

        // Set the modify bit for the node with watch_descriptor, as well as all
        // of the node's ancestors. 
        void SetModifyBit(int watch_descriptor);

        // Reset the modify bit for the node with watch_descriptor, as well as all
        // of the node's descendants.
        void ResetModifyBit(int watch_descriptor);

        // If the node with watch_descriptor does not exist in the directory tree,
        // return UPDATE_FLAG=DELETED. If the node has it's modify bit set, return
        // UPDATE_FLAG=MODIFIED. If neither of the aforementioned cases apply, return
        // UPDATE_FLAG=NOCHANGE.
        enum UPDATE_FLAG GetUpdateFlag(int watch_descriptor);
        boost::filesystem::path GetPath(int watch_descriptor);

    private:
        class Node {
            friend class DirectoryTree;
            private:
                int watch_descriptor_;
                boost::filesystem::path directory_path_;
                Node *parent_;
                std::list<Node*> children_;
                bool modified_;
            public:
                Node(int watch_descriptor, boost::filesystem::path directory_path);
                ~Node();
                void AddChild(DirectoryTree::Node *child) { children_.push_back(child); }
                void RemoveChild(DirectoryTree::Node *child) { children_.remove(child); }
                void SetParent(DirectoryTree::Node *parent) { parent_ = parent; }
                boost::filesystem::path GetPath() { return directory_path_; }
                int GetWatchDescriptor() { return watch_descriptor_; }
                int NumChildren() { return children_.size(); }
        };

        /* Maps watch descriptors to directory nodes */
        std::map<int, DirectoryTree::Node*> node_map_;
};

#endif
