#pragma once

/*
(c) 2024 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

/*This will be called once, after the system clock is initialized
  an operating system might even decide to not return and instead start a
  scheduler.
*/
void AppInit();

/*This will be called in an endless cycle. Its ok if this call never returns.
  If this function is not implemented, main.c provides a weak implementation,
  which is empty.
*/
void AppCycle();
