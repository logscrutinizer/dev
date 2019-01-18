#!/bin/bash
echo "Downloading ../build_cc/LogScrutinizer_v2_0_B0_build2-Linux-64bit.rpm"
wget www.logscrutinizer.com/downloads/../build_cc/LogScrutinizer_v2_0_B0_build2-Linux-64bit.rpm
echo "If you want to check if LogScrutinizer is already installed"
echo "you could test using"
echo "> rpm -qa | grep logscrutinizer"
echo "If you want to uninstall LogScrutinizer you can use"
echo "> sudo rpm -qa | grep logscrutinizer | xargs -0 -I file rpm -e file"
echo "Installing ... "
sudo rpm -i ../build_cc/LogScrutinizer_v2_0_B0_build2-Linux-64bit.rpm
