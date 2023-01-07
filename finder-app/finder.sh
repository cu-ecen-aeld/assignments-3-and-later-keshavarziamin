#!/bin/sh
# echo "the number of Arguments: $#"
if [ $# -eq 1 ] && [ $1 = '--help' ]  
then
    echo "Example invocation:"
    echo "  finder.sh /tmp/aesd/assignment1 linux "
    echo
    echo "the system print message:"
    echo "  The number of files are X and the number of matching lines are Y"
    exit 1
else 
    if [ $# -ne 2 ]
    then
        echo "the number of arguments is not valid"
        exit 1
    fi
fi
DIR=$1
search_str=$2

if [ ! -d "$DIR" ] 
then    
    echo "$DIR directory or file does not exit"
    exit 1
fi

#counting files in directory and subdirectories

file_number=$(find $DIR -type f| wc -l ) 
file_list=$(find $DIR -type f)
file_matched=0
for file in $file_list
do
    num=$(grep -c $search_str $file)
    file_matched=$((num+file_matched))
done
echo "The number of files are ${file_number} and the number of matching lines are ${file_matched}"