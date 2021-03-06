gtkollection
============

gtkollection is an application that lets you manage any kind of collection. Its
main goal is to centralize all your collections in one tool.

With gtkollection you can customize your collection by manipulating what fields
will be part of it.

For example, in a collection of music albums it may contain the following fields:

* Artist name
* Album name
* Gender
* Release year
* and many more...

It allows you to add an image for each entry your collection. These images can be
added from your local images or searched through the net. It uses an internal
plugin that can search the web using google image API.

License
-------

GPLv2

Dependencies
------------

To build gtkollection the following packages need to be installed on your system:

* make
* gcc
* libsqlite3-dev
* libgtk2.0-dev

To run it you will also need the following tools:

* convert (part of imagemagick package)
* python (version 2.7)

Build and Installation
----------------------

To build gtkollection just do:

* cd src
* make
* sudo make install

Ubuntu installation
-------------------

gtkollection has a launchpad.net repository for ubuntu distributions. Currently
it is supported on the following versions:

* Oneiric Ocelot (oneiric)
* Precise Pangolin (precise)
* Quantal Quetzal (quantal)

You can install gtkollection from its launchpad.net repository using the following
steps. First add its PPA to your system by copying the line below to your system's
software sources:

    deb http://ppa.launchpad.net/rsfreitas/gtkollection/ubuntu (YOUR UBUNTU VERSION HERE) main

Second add the PPA key signing key to your apt keyring:

    sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 0FEF6417
