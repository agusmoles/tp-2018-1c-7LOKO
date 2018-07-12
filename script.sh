#! /bin/bash

cd

cd /home/utnso/

sudo apt-get install gcc make libssl-dev libreadline6 libreadline-dev

git clone https://github.com/sisoputnfrba/so-commons-library.git

git clone https://github.com/sisoputnfrba/parsi.git

cd so-commons-library; sudo make install

cd ..

cd parsi; sudo make install

cd ..

clear
