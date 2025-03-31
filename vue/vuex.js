Vue.use(Vuex);
const store = new Vuex.Store({
  state: {
    sensors: {
      d: 0,
      f: 0,
    },
    history: [],
    results: [],
    lastResult: [],
    config: {},
    system: {},
    isConnected: false,
    authenticate: false,
    rootFiles: [],
  },
  mutations: {
    // update state

    updateSensors(state, obj) {
      state.sensors = obj;
    },
    updateConfig(state, obj) {
      state.config = obj;
    },
    updateSystem(state, obj) {
      state.system = obj;
    },
    updateHistory(state, obj) {
      state.history = obj;
    },
    updateResults(state, obj) {
      state.results = obj;
    },
    updateLastResult(state, obj) {
      state.lastResult = obj;
    },
    updateRootFiles(state,obj){
      state.rootFiles = obj;
    },

    //login
    authenticated(state, token) {
      state.authenticate = true;
      state.token = token;
      localStorage.setItem("token", token);
    },
    //logout
    logout(state) {
      state.authenticate = false;
      state.token = undefined;
      localStorage.removeItem("token");
    },
    //carga local
    loadLocal(state) {
      let token = localStorage.getItem("token");
      if (token) {
        state.authenticate = true;
        state.token = token;
      }
    },
    //se cierra la ventana
    unload(state) {
      if (this.connection !== undefined) {
        this.connection.close();
      }
    },
    //conected
    connected(state, bool) {
      state.isConnected = bool;
    } /**/,
  },
  actions: {
    // connexion inicial websocket
    connect({ commit, dispatch }) {
      if (this.connection !== undefined) {
        this.connection.close();
      }

      console.log(host);

      this.connection = new WebSocket("ws://" + host + "/ws");

      //delegamos los eventos a las acciones
      this.connection.onmessage = (event) => dispatch("onMessage", event);
      this.connection.onclose = (event) => dispatch("onClose", event);
      this.connection.onerror = (event) => dispatch("onError", event);
      this.connection.onopen = (event) => dispatch("onOpen", event);
    },
    //reiniciar esp32
    restart({ dispatch }) {
      dispatch("send", {
        cmd: { restar: 1 },
      });
    },
    // intento de autentificacion
    setAuthentication({ dispatch, commit, state }, user) {
      if (this.connection !== undefined) {
        this.connection.send(JSON.stringify({ login: user }));
      } else dispatch("connect");
    },
    // recibe wasaps del servidor de eventos/errores
    // hay q suscribirse a vuex para tratar los mensajes
    notify({ commit, state }, notify) {
      //is event
    },
    // para manejar el routing desde servidor y vuex
    // podemos abrir paginas al usuario
    // hay q suscribirse a vuex
    goTo({ commit, state }, path) {
      //is event
    },
    // para cerrar la session del usuario desde el servidor
    logoutUser({ dispatch, commit, state }, path) {
      commit("logout");
      dispatch("goTo", "/login");
    },
    /***********************
     *      data
     ***********************/
    //para descargar datos publicos
    // [ "/result/pla.json", "/result/abs.json" ]
    loadResults({ dispatch }, indexes) {
      dispatch("send", { loadData: { indexes: indexes } });
    },

    //para descargar datos autenticadas
    //pageId = 0 option | pageId = 1 system
    loadDataAuth({ dispatch }, pageId) {
      dispatch("send", { loadDataAuth: pageId });
    },

    editConfig({ commit, dispatch }, obj) {
      dispatch("send", { config: obj });
    },

    sendCmd({ commit, dispatch }, obj) {
      dispatch("send", { cmd: obj });
    },

    //envia objectos/comandos en json al servidor
    send({ commit, dispatch, state }, obj) {
      // si estamos identificado
      if (state.authenticate) {
        //metemos el token
        obj.token = state.token;
      }

      if (this.connection !== undefined) {
        this.connection.send(JSON.stringify(obj));
      } else dispatch("connect");
    },

    //file
    async downloadItem({ commit, dispatch, state }, name) 
    {
      try {
        const response = await axios.get(`http://${host}/file`, {
          headers: { 'Authorization': `Basic ${state.token}` },
          params: { download: name },
          responseType: 'blob',
        });
    
        const blob = new Blob([response.data], { type: 'application/*' });
    
        // Verificar compatibilidad con la API File System Access
        if (window.showSaveFilePicker) {
          try {
            // Mostrar el diálogo de guardado
            const fileHandle = await window.showSaveFilePicker({
              suggestedName: name.split("/").pop(),
              types: [
                {
                  description: 'Archivos',
                  accept: { 
                    'application/json': ['.json'], 
                    'application/zip': ['.gz'],
                    'text/plain': ['.txt'],
                   },
                },
              ],
            });
    
            // Escribir el archivo en el sistema
            const writableStream = await fileHandle.createWritable();
            await writableStream.write(blob);
            await writableStream.close();
    
          } catch (saveError) {
            dispatch("message", { type: 1, content: `Download error ${saveError.message}` });
          }
        } else {
          // Fallback para navegadores que no soportan File System Access API
          const link = document.createElement('a');
          link.href = URL.createObjectURL(blob);
          link.download = name.split("/").pop();
          link.style.visibility = "hidden";
    
          // Añadir el enlace al DOM y hacer clic en él
          document.body.appendChild(link);
          link.click();
    
          // Limpiar y eliminar el enlace
          document.body.removeChild(link);
          URL.revokeObjectURL(link.href);
        }
      } catch (error) {
        dispatch("message", { type: 1, content: `Download error ${name} ${error.message}` });
      }
    },
    
    async deleteItem ({commit, dispatch,state },name) {
  
      await axios.get(`http://${host}/file`, {          
        headers: { 'Authorization': `Basic ${state.token}`},
        params: {delete:name},
      }).then(response => {
          dispatch("message",{type:0,content:`Deleted successfully ${name.split("/").pop()}`});
        }).catch((error) => {
          console.error(error);
          dispatch("message",{type:1,content:`Error deleting ${name} ${error.message}`});
      });
    },

    async uploadItem({commit, dispatch,state },{file,params}){
      let formData = new FormData();
      formData.append('file', file);
      let filename = file;
      await axios.post( `http://${host}/file`,
        formData,{
          headers: {
            'Authorization': `Basic ${state.token}`,
            'Content-Type': 'multipart/form-data',
          },
          params: params,
        }
        ).then((res) => {
          dispatch("message",{type:0,
            content:`Uploaded successfully ${file?file.name.split("/").pop():''}`});
        })
        .catch((error) => {
          console.log(error);
          dispatch("message",{type:1,
            content:`Error uploading ${file?file.name:'no difinido'}  ${error.message}`});
        });
    },

    //privado
    // eventos websocket
    //json recibido del Esp32
    onMessage({ commit, dispatch }, event) {
      //convierte json en un objecto js
      let json = JSON.parse(event.data);
      //console.log(json);
      // TODO mutations
      const mutations = {
        ss: "updateSensors",
        token: "authenticated",
        system: "updateSystem",
        config: "updateConfig",
        history: "updateHistory",
        results: "updateResults",
        lastResult: "updateLastResult",
        root: "updateRootFiles",
      };
      //actualizamos los datos por commit("mutation")
      for (const key in mutations) {
        if (json[key] !== undefined) 
          commit(mutations[key], json[key]);
      }

      const actions = {
        message: "notify",
        goTo: "goTo",
        logout: "logoutUser",
      };
      //ejecutamos acciones dispatch("action")
      for (const key in actions) {
        if (json[key] !== undefined) dispatch(actions[key], json[key]);
      }
    },
    onClose({ commit, dispatch }, event) {
      // ponemos state.isConnected a false
      commit("connected", false);
      //reconectamos al pasar 3sg
      setTimeout(() => {
        dispatch("connect");
      }, 3000);
    },
    onError({ commit, state }, event) {
      //is event
    },
    onOpen({ commit, state }, event) {
      commit("connected", true);
    } /**/,
  },
  getters: {
    getHistoryIndex: (state) => (name) => {
      return state.history.findIndex((item) => item.name === name);
    },
    // Obtener la fecha y hora actual
    getCurrentDate: (state) => () => {
      const now = new Date();

      // Extraer día, mes, año, horas y minutos
      const d = String(now.getDate()).padStart(2, "0"); // Día con dos dígitos
      const m = String(now.getMonth() + 1).padStart(2, "0"); // Mes con dos dígitos (los meses van de 0 a 11)
      const y = now.getFullYear(); // Año con cuatro dígitos
      const h = String(now.getHours()).padStart(2, "0"); // Horas con dos dígitos
      const min = String(now.getMinutes()).padStart(2, "0"); // Minutos con dos dígitos

      // Formatear la fecha y hora en el formato deseado
      return `${d}/${m}/${y} ${h}:${min}`;
    },
  },
  strict: true,
});
