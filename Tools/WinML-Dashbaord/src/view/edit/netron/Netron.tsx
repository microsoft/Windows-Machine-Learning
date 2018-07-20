import * as React from 'react';
import { connect } from 'react-redux';

import 'netron/src/view-render.css';
import 'netron/src/view-sidebar.css';
import 'netron/src/view.css';
import 'npm-font-open-sans/open-sans.css';

import { updateGraph, updateInputs, updateMetadataProps, updateProperties } from '../../../datastore/actionCreators';
import IState from '../../../datastore/state';
import './fixed-position-override.css';

const browserGlobal = window as any;

interface IComponentProperties {
    file?: File,

    // Redux properties
    updateGraph: typeof updateGraph,
    updateInputs: typeof updateInputs,
    updateMetadataProps: typeof updateMetadataProps,
    updateProperties: typeof updateProperties,
}

interface IComponentState {
    graph: any,
    metadataProps: { [key: string]: string },
    properties: { [key: string]: string },
}

class NetronComponent extends React.Component<IComponentProperties, IComponentState> {
    private root: React.RefObject<HTMLDivElement>;
    private revokeModelLoadedProxy: () => void;

    constructor(props: IComponentProperties) {
        super(props);
        this.state = {
            graph: {},
            metadataProps: {},
            properties: {},
        }
    }

    public componentDidMount() {
        // Netron must be run after rendering the HTML
        if (!browserGlobal.host) {
            const s = document.createElement('script');
            s.src = "netron_bundle.js";
            s.async = true;
            s.onload = this.installModelLoadedProxy;
            document.body.appendChild(s);
        } else {
            this.installModelLoadedProxy();
        }
    }

    public componentWillUnmount() {
        if (this.revokeModelLoadedProxy) {
            this.revokeModelLoadedProxy();
        }
    }

    public UNSAFE_componentWillReceiveProps(nextProps: IComponentProperties) {
        if (!browserGlobal.host) {
            return;
        }
        if (nextProps.file !== this.props.file) {
            browserGlobal.host._openFile(nextProps.file);
        }
    }

    public shouldComponentUpdate(netxtProps: IComponentProperties, nextState: IComponentState) {
        // Netron is a static page and all updates are handled by its JavaScript code
        return false;
    }

    public render() {
        return (
            <div ref={this.root} style={{height: '100%'}}>
                <div id='welcome' className='background' style={{display: 'block'}}>
                    <div className='center logo'>
                        <img className='logo absolute' src='logo.svg' />
                        <img id='spinner' className='spinner logo absolute' src='spinner.svg' style={{display: 'none'}} />
                    </div>
                    <button id='open-file-button' className='center' style={{top: '200px', width: '125px', opacity: 0}}>Open Model...</button>
                    <input type="file" id="open-file-dialog" style={{display: 'none'}} multiple={false} accept=".onnx, .pb, .meta, .tflite, .keras, .h5, .json, .mlmodel, .caffemodel" />
                    <div style={{fontWeight: 'normal', color: '#e6e6e6', userSelect: 'none'}}>.</div>
                    <div style={{fontWeight: 600, color: '#e6e6e6', userSelect: 'none'}}>.</div>
                    <div style={{fontWeight: 'bold', color: '#e6e6e6', userSelect: 'none'}}>.</div>
                </div>
                <svg id='graph' className='graph' preserveAspectRatio='xMidYMid meet' width='100%' height='100%' />
                <div id='toolbar' className='toolbar' style={{position: 'absolute', top: '10px', left: '10px', display: 'none',}}>
                    <button id='model-properties-button' className='xxx' title='Model Properties'>
                        <svg viewBox="0 0 100 100" width="24" height="24">
                            <rect x="12" y="12" width="76" height="76" rx="16" ry="16" strokeWidth="8" stroke="#fff" />
                            <line x1="30" y1="37" x2="70" y2="37" strokeWidth="8" strokeLinecap="round" stroke="#fff" />
                            <line x1="30" y1="50" x2="70" y2="50" strokeWidth="8" strokeLinecap="round" stroke="#fff" />
                            <line x1="30" y1="63" x2="70" y2="63" strokeWidth="8" strokeLinecap="round" stroke="#fff" />
                            <rect x="12" y="12" width="76" height="76" rx="16" ry="16" strokeWidth="4" />
                            <line x1="30" y1="37" x2="70" y2="37" strokeWidth="4" strokeLinecap="round" />
                            <line x1="30" y1="50" x2="70" y2="50" strokeWidth="4" strokeLinecap="round" />
                            <line x1="30" y1="63" x2="70" y2="63" strokeWidth="4" strokeLinecap="round" />
                        </svg>
                    </button>
                    <button id='zoom-in-button' className='icon' title='Zoom In'>
                        <svg viewBox="0 0 100 100" width="24" height="24">
                            <circle cx="50" cy="50" r="35" strokeWidth="8" stroke="#fff" />
                            <line x1="50" y1="38" x2="50" y2="62" strokeWidth="8" strokeLinecap="round" stroke="#fff" />
                            <line x1="38" y1="50" x2="62" y2="50" strokeWidth="8" strokeLinecap="round" stroke="#fff" />
                            <line x1="78" y1="78" x2="82" y2="82" strokeWidth="12" strokeLinecap="square" stroke="#fff" />
                            <circle cx="50" cy="50" r="35" strokeWidth="4" />
                            <line x1="50" y1="38" x2="50" y2="62" strokeWidth="4" strokeLinecap="round" />
                            <line x1="38" y1="50" x2="62" y2="50" strokeWidth="4" strokeLinecap="round" />
                            <line x1="78" y1="78" x2="82" y2="82" strokeWidth="8" strokeLinecap="square" />
                        </svg>
                    </button>
                    <button id='zoom-out-button' className='icon' title='Zoom Out'>
                        <svg viewBox="0 0 100 100" width="24" height="24">
                            <circle cx="50" cy="50" r="35" strokeWidth="8" stroke="#fff" />
                            <line x1="38" y1="50" x2="62" y2="50" strokeWidth="8" strokeLinecap="round" stroke="#fff" />
                            <line x1="78" y1="78" x2="82" y2="82" strokeWidth="12" strokeLinecap="square" stroke="#fff" />
                            <circle cx="50" cy="50" r="35" strokeWidth="4" />
                            <line x1="38" y1="50" x2="62" y2="50" strokeWidth="4" strokeLinecap="round" />
                            <line x1="78" y1="78" x2="82" y2="82" strokeWidth="8" strokeLinecap="square" />
                        </svg>
                    </button>
                </div>
                <div id='sidebar' className='sidebar'>
                    <h1 id='sidebar-title' className='sidebar-title' />
                    <a href='javascript:void(0)' id='sidebar-closebutton' className='sidebar-closebutton'>&times;</a>
                    <div id='sidebar-content' className='sidebar-content' />
                </div>
            </div>
        );
    }

    private propsToObject(props: any) {
        return props.reduce((acc: { [key: string]: string }, x: any) => {
            // metadataProps uses key, while model.properties uses name
            acc[x.key || x.name] = x.value;
            return acc;
        }, {});
    }

    private installModelLoadedProxy = () => {
        document.documentElement.style.overflow = 'initial';
        // Install proxy on browserGlobal.view.updateGraph and update the data store
        const handler = {
            apply: (target: any, thisArg: any, args: any) => {
                const model = args[0];
                // FIXME What to do when model has multiple graphs?
                const graph = model.graphs[0];
                this.props.updateInputs(graph.inputs);
                // Normalize graph
                // const normalizedGraph: Array<{}> = [];
                // for (const node of graph.nodes) {
                //     normalizedGraph.push({
                //         inputs: node.inputs,
                //         // ... Add other properties of interest
                //     });
                // }
                // this.props.updateGraph(normalizedGraph);
                this.props.updateMetadataProps(this.propsToObject(model._metadataProps));
                this.props.updateProperties(this.propsToObject(model.properties));
                return target.apply(thisArg, args);
            },
        };
        const revokableProxy = Proxy.revocable(browserGlobal.view.updateGraph, handler);
        browserGlobal.view.updateGraph = revokableProxy.proxy;
        this.revokeModelLoadedProxy = revokableProxy.revoke;
    }
}

const mapStateToProps = (state: IState) => ({
    graph: state.graph,
    metadataProps: state.metadataProps,
    properties: state.properties,
});

const mapDispatchToProps = {
    updateGraph,
    updateInputs,
    updateMetadataProps,
    updateProperties,
}

export const Netron = connect(mapStateToProps, mapDispatchToProps)(NetronComponent);
