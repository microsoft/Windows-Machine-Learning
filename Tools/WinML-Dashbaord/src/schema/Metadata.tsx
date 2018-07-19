export default {
    // https://github.com/onnx/onnx/blob/master/docs/MetadataProps.md
    // FIXME be case insensitive
    type: 'object',

    additionalProperties: false,
    patternProperties: {
        '.+': {
            type: 'string',
        },
    },
    properties: {
        'Image.BitmapPixelFormat': {
            enum: [
                'Gray8',
                'Rgb8',
                'Bgr8',
                'Rgba8',
                'Bgra8',
            ],
            type: 'string',
        },
        'Image.ColorSpaceGamma': {
            enum: [
                'Linear',
                'SRGB',
            ],
            type: 'string',
        },
        'Image.NominalPixelRange': {
            enum: [
                'NominalRange_0_255',
                'Normalized_0_1',
                'Normalized_1_1',
                'NominalRange_16_235',
            ],
            type: 'string',
        },
        model_author: {
            type: 'string',
        },
        model_license: {
            type: 'string',
        },
    },
};
