# Repository setup

Using the package app-eselect/eselect-repository.

```
sudo eselect repository add ventura-repo git@github.com:jorgeventura/ventura-repo.git
```

# After clone project

 1. cd <project dir>
 2. $ git submodule init

To fetch the content:

 3. $ git submodule update


# Regular work with project

 1. Make changes
 2. compile using make program

 3. $ make distcheck
 4. $ make create-ebuild
 5. $ sudo make update-manifest

 6. $ cd rem-repo
    6.1 $ git status
    6.2 $ git add <new untracked>.ebuild
    6.3 $ git commit -am'version n.n.n'
    6.4 $ git push
    6.5 $ cd ..

 7. $ git commit -am'<custom message>'
 8. $ git push


