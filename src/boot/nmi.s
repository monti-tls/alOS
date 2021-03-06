/*
 * alOS
 * Copyright (C) 2015 Alexandre Monti
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

.syntax unified
.cpu cortex-m4
.thumb

.type  irq_nmi_handler, %function
irq_nmi_handler:
	b .

.type  irq_hardfault_handler, %function
irq_hardfault_handler:
	b .

.type  irq_memmanage_handler, %function
irq_memmanage_handler:
	b .

.type  irq_busfault_handler, %function
irq_busfault_handler:
	b .

.type  irq_usagefault_handler, %function
irq_usagefault_handler:
	b .
