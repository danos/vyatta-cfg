/*
	Copyright (c) 2020AT&T Intellectual Property.

	SPDX-License-Identifier: GPL-2.0-only
*/

#include "CppUTest/CommandLineTestRunner.h"
extern "C"
{
}

int main(int ac, char **av)
{
	return CommandLineTestRunner::RunAllTests(ac, av);
}
