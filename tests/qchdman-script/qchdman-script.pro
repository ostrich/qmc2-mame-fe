TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += fake-chdman harness

fake-chdman.file = fake-chdman/fake-chdman.pro
harness.file = harness/harness.pro
harness.depends = fake-chdman
