#ifndef DIRECTORY_TREE_H
#define DIRECTORY_TREE_H

#include <string>
#include <list>
#include <map>
#include "boost/filesystem.hpp"

enum UPDATE_FLAG { MODIFIED, DOESNOTEXIST, NOCHANGE };


class DirectoryTree {
    public:
        DirectoryTree();
        ~DirectoryTree();

        void AddNode(int watch_descriptor, boost::filesystem::path directory_path);
        void RemoveNode(boost::filesystem::path directory_path);
        void RemoveNode(int watch_descriptor);
        boost::filesystem::path GetPath(int watch_descriptor);
        //void GetNode(boost::filesystem::path directory_path);

        // Set the modify bit for the node with watch_descriptor, as well as all
        // of the node's ancestors. 
        void SetModifyBit(int watch_descriptor);

        // Reset the modify bit for the node with directory_path, as well as all
        // of the node's descendants.
        void ResetModifyBit(boost::filesystem::path directory_path);

        // If the node with watch_descriptor does not exist in the directory tree,
        // return UPDATE_FLAG=DOESNOTEXIST. If the node has it's modify bit set, return
        // UPDATE_FLAG=MODIFIED. If neither of the aforementioned cases apply, return
        // UPDATE_FLAG=NOCHANGE.
        enum UPDATE_FLAG GetUpdateFlag(boost::filesystem::path directory_path);

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
                bool HasParent() { return (parent_ == NULL) ? false : true; }
                void SetParent(DirectoryTree::Node *parent) { parent_ = parent; }
                void UnsetParent() { parent_ = NULL; }
                boost::filesystem::path GetPath() { return directory_path_; }
                int GetWatchDescriptor() { return watch_descriptor_; }
                int NumChildren() { return children_.size(); }
        };

        std::map<std::string, DirectoryTree::Node*> path_to_node_;
        std::map<int, DirectoryTree::Node*> wd_to_node_;
};

#endif
