#!/bin/sh
file="log.txt"
error_file="error.txt"
date > $file
echo Testing files
for doss in `ls -d */`
do
    printf "\n->looking at $doss DIRECTORY\n" >> $file
    cd $doss
    for fich in `ls . `
    do
        if [ -f $fich ]
        then
       	    cd ../..
            ./bin/compil < tests/$doss/$fich  2> tests/$error_file
        	if [ $? = 1 ]
        	then 
                echo "\n"FILE " ": $doss$fich >> tests/$file "\n"STATE : ERROR 
            else 
                echo "\n"FILE " ": $doss$fich >> tests/$file "\n"STATE : CORRECT 
                cd src
                make > /dev/null
                cd .. 
                echo "executing $doss$fich :"
                ./bin/result.out 
                echo "\n-------------------------"
            fi
            cat tests/$error_file >> tests/$file
            cd tests/$doss
        fi
    done
    cd ..
done
rm $error_file
echo Tests are done look at the file with the command : cat log.txt
