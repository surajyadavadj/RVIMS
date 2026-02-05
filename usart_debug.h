/* usart_debug.h -- USART2 debug print functions
 */
#ifndef USART_DEBUG_H
#define USART_DEBUG_H

// USART2 Initialization (includes GPIO setup)
void USART2_Init(void);

// USART2 Debug print function (printf-style)
void usart_debug(const char* format, ...);

#endif // USART_DEBUG_H
