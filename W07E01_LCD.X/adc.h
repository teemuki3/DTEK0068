#ifndef ADC_H
#define ADC_H

/* Makes all required inital configurations for ADC usage. */
void adc_init(void);
 
/* Performs a single ADC conversion. */
uint16_t adc_converse(void);

/* Mutex-protected function, which first makes all necessary ADC configurations 
 * for the given input before calling adc_converse(). */
uint16_t adc_read(uint8_t input);

// Possible inputs for adc_read
#define LDR 0
#define NTC 1
#define POT 2

#endif /* ADC_H */