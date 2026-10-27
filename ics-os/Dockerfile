############################################################
# ICS-OS Development
############################################################
FROM ubuntu:16.04

RUN apt-get update

RUN apt-get install -y build-essential nasm qemu-kvm tcc git gcc-multilib sudo

RUN mkdir -p /home/ics-os
WORKDIR /home/ics-os/ics-os

RUN cd /home/ics-os/ics-os/

RUN git config --global --add safe.directory /home/ics-os

cd /home/ics-os/ics-os

