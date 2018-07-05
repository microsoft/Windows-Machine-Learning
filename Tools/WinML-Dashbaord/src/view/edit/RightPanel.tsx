import * as Ajv from 'ajv';
import { JsonEditor } from 'jsoneditor-react';
import 'jsoneditor-react/es/editor.min.css';
import { Label } from 'office-ui-fabric-react/lib/Label';
import * as React from 'react';

import Collapsible from '../../components/Collapsible';
import MetadataSchema from '../../schema/Metadata';

const ajv = new Ajv({ allErrors: true, verbose: true });

export default class RightPanel extends React.Component {
    render() {
        console.warn(MetadataSchema);
        return (
            <div>
                <Label>Model</Label>
                <Collapsible label='Model metadata'>
                    <JsonEditor
                        ajv={ajv}
                        schema={MetadataSchema}
                        value={{
                            a: 'string',
                            b: 2,
                        }}
                        // onChange={this.handleChange}
                    />
                </Collapsible>
            </div>
        );
    }
}
