# README for vyatta-cfg Unit Tests

vyatta-cfg has unit tests that use CppUTest.  These run automatically as
part of the build, and failure will stop the build.  They can also be run
manually, as shown below.

Use of the 'my_tools' repo in BitBucket makes the setup easier...

* mkdir ~/buildroots
* cd buildroots
* setup-chroot <chroot-name> vyatta-cfg <prj>
  * eg setup-chroot master vyatta-cfg Vyatta:Master
  * this will perform initial build
* set-chroot <chroot-name>
* cd work/vyatta-cfg
* make check

You can run this TDD-style as follows:

* sudo apt install entr
* cd /work/vyatta-cfg
* find . -name "*.[ch]*" | entr make check

If a specific test isn't working, then until I've worked out how to get the
'-v' option passed in via automake, you will need to run this to see the
output:

* make check
* ./cpputest/connectTester (or whichever test it is)
