To create new error definitions for Thor, do the following:

1. Create new error codes and (if needed) error sources in
   ThorError.json.

2. Run the script generror.sh.  This will generate a
   C/C++ header file and a Java class.

   Header file location:

	device/echonous/thor/apps/common/include/ThorError.h.
   
    Java class file location:

	device/echonous/thor/apps/UltrasoundManager/java/com/echonous/hardware/ultrasound/ThorError.java 

3. Do a build of the entire source tree.  

4. Check in the modified ThorError.json file AND BOTH generated files into Gerrit

