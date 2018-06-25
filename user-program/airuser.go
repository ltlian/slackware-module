package main

import (
	"log"
	"fmt"
	"io/ioutil"
	"os"
	"strconv"
	"strings"
)

var devFile = `/dev/airtune`
var sampleSize = 100	// sample size for averaging
var increments = 10		// number of increments for regression

// Read/Write enums
const (
    r accessmode = iota
    w
)

type accessmode int

// command contains the command type & payload
type command struct {
	field string
	accessmode
}

// regression is a helper struct used during the calibration process
type regression struct {
	xsum float64
	ysum float64
	x2sum float64
	xysum float64
}

// gain() returns the sensor module's gain after the regression struct has been
// initialized with the necessary values.
func (rg regression) gain() float64 {
	numerator := (float64(increments) * rg.xysum) - (rg.xsum * rg.ysum)
	denominator := (float64(increments) * rg.x2sum) - (rg.xsum * rg.xsum)
	return numerator / denominator
}

// offset() returns the sensor module's offset after the regression struct has
// been initialized with the necessary values.
func (rg regression) offset() float64 {
	numerator := rg.ysum*rg.x2sum - rg.xsum*rg.xysum
	denominator := (float64(increments) * rg.x2sum) - (rg.xsum * rg.xsum)
	return numerator / denominator
}

// calibrate() averages a set of values from the kernel module and returns its
// gain and offset
func calibrate() (float64, float64) {
	var rg regression
	var xi float64
	var yi float64

	for i := 0; i <= increments; i++ {
		write(fmt.Sprintf("R=%v", i*1000))
		xi = float64(i)
		yi = getAverages() / 1000
		rg.ysum += yi
		rg.x2sum += xi * xi
		rg.xsum += xi
		rg.xysum += (xi * yi)
	}

	return rg.gain(), rg.offset()
}

// read() is used by getAverages() to get a sample reading from the device
func read() int {
	var res []byte
	res = make([]byte, 8)
	res, err := ioutil.ReadFile(devFile)
	if err != nil {
		fmt.Printf("[airuser.go] Could not open %v\n", devFile)
		log.Fatal(err)
	}

	// Strip up to 50 newlines
	resStr := strings.Replace(string(res), "\n", "", 50)

	resNum, err := strconv.Atoi(resStr)
	if err != nil {
		fmt.Printf("[airuser.go] Error parsing string while reading from %v,\n%v\n", devFile, err)
		log.Fatal(err)
	}

	return resNum
}

// write(arg) writes a string to the kernel module as a means of passing
// commands and parameters
func write(arg string) error {
	f, err := os.OpenFile(devFile, os.O_APPEND|os.O_WRONLY, os.ModeAppend)
	defer f.Close()

	if err != nil {
		fmt.Printf("[airuser.go] Error opening device\n%v\n", err)
		return err
	}

	_, err = f.WriteString(arg)
	if err != nil {
		fmt.Printf("[airuser.go] Error writing to device\n%v\n", err)
		return err
	}

	return nil
}

// getAverages() is used during the calibration process and returns the average
// of 'sampleSize' sensorvalues from the device.
func getAverages() float64 {
	var sumReadings int
	for i := 0; i < sampleSize; i++ {
		sumReadings += read()
	}

	avgReadings := float64(sumReadings) / float64(sampleSize)
	return avgReadings
}

// setInitialValues(G, O) initializes the device with an arbitrary gain and
// offset such that these values are known.
func setInitialValues(G int, O int) {
	fmt.Printf("Setting initial values:\n")

	write(fmt.Sprintf("G=%d", G))
	fmt.Printf("\t%-18s %5d\n", "Gain set to", G)

	write(fmt.Sprintf("O=%d", O))
	fmt.Printf("\t%-18s %5d\n", "Offset set to", O)
}

// parseArgs(arg) parses a string argument into a command struct consisting of
// a read/write selector and an optional write payload
func parseArgs(arg []string) (*command, error) {
	var cmd command

	// if commands + 1 < 2, parse as read command
	if len(arg) < 2 {
		cmd.accessmode = r
		return &cmd, nil
	}

	if len(arg) > 2 {
		return nil, fmt.Errorf("airuser.go only takes one optional argument")
	}

	cmd.accessmode = w
	cmd.field = arg[1]
	return &cmd, nil
}

func main() {
	cmd, err := parseArgs(os.Args)
	if err != nil {
		log.Fatal(err)
	}

	switch cmd.accessmode {
	case w:
		fmt.Printf("Passing '%v' to %v\n", cmd.field, devFile)
		err := write(cmd.field)
		if err != nil {
			log.Fatal(err)
		}

	case r:
		setInitialValues(1000, 0)
		gain, offset := calibrate()
		fmt.Printf("Results:\n")
		fmt.Printf("\t%-22s %4.2f\n", "Gain:", gain)
		fmt.Printf("\t%-21s %4.2f\n", "Offset:", offset)
	}
}
