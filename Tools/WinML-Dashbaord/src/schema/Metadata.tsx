export default {
    // https://github.com/onnx/onnx/blob/master/docs/MetadataProps.md
    // FIXME be case insensitive
    type: 'object',
    properties: {
        model_author: {
            type: 'string',
        },
        model_license: {
            type: 'string',
        },
        'Image.BitmapPixelFormat': {
            type: 'string',
            enum: [
                'Gray8',
                'Rgb8',
                'Bgr8',
                'Rgba8',
                'Bgra8',
            ],
        },
        'Image.ColorSpaceGamma': {
            type: 'string',
            enum: [
                'Linear',
                'SRGB',
            ],
        },
        'Image.NominalPixelRange': {
            type: 'string',
            enum: [
                'NominalRange_0_255',
                'Normalized_0_1',
                'Normalized_1_1',
                'NominalRange_16_235',
            ],
        },
    },
};
