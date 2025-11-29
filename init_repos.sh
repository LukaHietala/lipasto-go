#!/bin/sh

# create a bare repo for testing

set -e 

# cleanup
rm -rf /tmp/bare-repo /tmp/temp-repo

# create a bare repository
mkdir -p /tmp/bare-repo
cd /tmp/bare-repo
git init --bare
cd /tmp

# make temp repo to make commits to bare repo
mkdir -p /tmp/temp-repo
cd /tmp/temp-repo
git init

# create local user 
git config user.name "mirri"
git config user.email "mirri@proton.me"

# new file
echo "ABC hiiri ajelee pikkuisissa rattahissa eipä niihin mahdu kissa" > loru.txt
git add loru.txt
git commit -m "init"

# other new file
echo "ABC kissa kävelee päällä uusi hieno takki päässä keikkuu piippalakki." > loru2.txt
git add loru2.txt
git commit -m "new file"

# edit file
echo "ABC kissa kävelee tikapuita pitkin taivaaseen." > loru.txt
git add loru.txt
git commit -m "edit file"

# remove file
rm loru2.txt
git add .
git commit -m "remove file"

# add remote and push
git remote add origin /tmp/bare-repo
git push origin master

# cleanup
cd /tmp
rm -rf /tmp/temp-repo

echo "bare repository created at /tmp/bare-repo with four commits on the master bran"
