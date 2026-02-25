    private final int code;
    private final String name;
    private final String description;

    // Build a hash table to map error codes back to the Enum.
    private static final ThorError[] values = ThorError.values();

    private static final HashMap<Integer, ThorError> valToErrorMap = 
        new HashMap<Integer, ThorError>();

    static {
        for (ThorError error : values) {
            valToErrorMap.put(error.getCode(), error);
        }
    }

    private ThorError(int code, String name, String description) {
        this.code = code;
        this.name = name;
        this.description = description;
    }

    /**
     * Returns a ThorError instance from a raw error code.  Will return THOR_ERROR 
     * if the specified raw code is invalid. 
     * 
     * 
     * @param code The raw error code
     * 
     * @return The ThorError enumeration
     */
    public static ThorError fromCode(int code) {
        ThorError   error = valToErrorMap.get(code);

        if (null == error) {
            // A bogus code was submitted, so make a generic error.
            error = THOR_ERROR;
        }
        return error;
    }

    /**
     * Returns the raw error code.
     * 
     */
    public int getCode() {
        return code;
    }

    /**
     * Returns the name of this error.
     * 
     */
    public String getName() {
        return name;
    }

    /**
     * Returns the description of this error.
     * 
     */
    public String getDescription() {
        return description;
    }

    /**
     * Returns a localized description of this error.
     * 
     * @param context The application context for retrieving the localized string 
     *                resource. The name of the resource string should be the same
     *                as the ThorError enumeration.
     * 
     * @return String The localized description.  If one does not exist, then null 
     *         is returned.
     */
    public String getLocalizedMessage(Context context) {
        String  message = null;
        int     resId;

        resId = context.getResources().getIdentifier(this.name(),
                                                     "string",
                                                     context.getPackageName());
        if (0 != resId) {
            message = context.getString(resId);
        }

        return message;
    }

    /**
     * Returns the Source of this error.
     * 
     */
    public ThorError getSource() {
        // Error Sources, OK, and ERROR don't have parent sources.
        if (code <= THOR_ERROR.getCode()) {
            return this;
        }
        else {
            // Mask off everything except the Error Source bits
            return fromCode((code & 0xFFF0000) >> 16);
        }
    }

    /**
     * Returns true if this is a success code, false if an actual error.
     * 
     */
    public boolean isOK() {
        return ((code & THOR_ERROR.getCode()) != THOR_ERROR.getCode());
    }


    /**
     * Returns a string that can be used for display or logging purposes.  The 
     * format is <code>&ltname&gt (raw code): &ltdescription&gt</code>.
     * 
     */
    @Override
    public String toString() {
        return name + "(" + Integer.toHexString(code) + "): " + description;
    }
