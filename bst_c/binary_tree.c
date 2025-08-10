#include <stdio.h>
#include <stdlib.h>

// This code doesn't handle the free, since it's just a trivial example.

struct node
{
    int val;
    struct node *left, *right;
};

/** Add a new node, if root is NULL return the new root
 * (the first inserted node) otherwise it just returns
 * the root passed by the user in input*/
struct node *add(struct node *root, int val)
{
    struct node *saved_root = root;
    struct node *new = malloc(sizeof(*new));
    new->left = new->right = NULL;
    new->val = val;

    if (root == NULL)
        return new;

    while (1)
    {
        if (val < root->val)
        {
            if (root->right == NULL)
            {
                root->right = new;
                return saved_root;
            }
            root = root->right;
        }
        else
        {
            if (root->left == NULL)
            {
                root->left = new;
                return saved_root;
            }
            root = root->left;
        }
    }
}

void print_sorted(struct node *root)
{
    if (root == NULL)
        return;
    print_sorted(root->left);
    printf("%d\n", root->val);
    print_sorted(root->right);
}

int main(void)
{
    struct node *root = NULL;

    root = add(root, 10);
    root = add(root, 5);
    root = add(root, 120);
    root = add(root, 40);
    root = add(root, 33);
    root = add(root, 2);
    root = add(root, 11);
    root = add(root, 23);

    print_sorted(root);
    return 0;
}
